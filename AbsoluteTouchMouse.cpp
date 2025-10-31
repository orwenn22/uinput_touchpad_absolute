#include "AbsoluteTouchMouse.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sched.h>

#include "main.h"
#include "utils.h"


AbsoluteTouchMouse::AbsoluteTouchMouse() {
    uinput_fd = -1;
    pad_fd = -1;
    enabled = true;
    verbose = false;

    x_updated = 0;
    y_updated = 0;
    absolute_max_x = PAD_MAX_X;
    absolute_max_y = PAD_MAX_Y;
    absolute_x = 0;
    absolute_y = 0;
    clamped_absolute_x = 0;
    clamped_absolute_y = 0;

    screen_width = SCREEN_WIDTH;
    screen_height = SCREEN_HEIGHT;

    enable_area = false;
    area_min_x = PAD_REGION_MIN_X;
    area_min_y = PAD_REGION_MIN_Y;
    area_max_x = PAD_REGION_MAX_X;
    area_max_y = PAD_REGION_MAX_Y;

    last_down = {0};
    tap_active = 0;

    show_pulling_rate = false;
    gettimeofday(&pulling_rate_start, nullptr);
    pulling_rate = 0;
    pulling_rate_tmp = 0;
}
AbsoluteTouchMouse::~AbsoluteTouchMouse() {
    DestroyUInputDevice();
    CloseTouchpad();
}

void AbsoluteTouchMouse::CreateUInputDevice() {
    if (uinput_fd >= 0) DestroyUInputDevice();

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Cannot open /dev/uinput");
        exit(EXIT_FAILURE);
    }

    //enable event types (allow clicking)
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);

    //this makes our fake device act as a tablet, which mean absolute events will be
    //treated as mouse positions
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    //ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER); //idk, useless?

    //configure absolute axes ranges
    struct uinput_abs_setup abs_x = {
        .code = ABS_X,
        .absinfo = { .minimum = 0, .maximum = SCREEN_WIDTH }
    };
    struct uinput_abs_setup abs_y = {
        .code = ABS_Y,
        .absinfo = { .minimum = 0, .maximum = SCREEN_HEIGHT }
    };
    ioctl(fd, UI_ABS_SETUP, &abs_x);
    ioctl(fd, UI_ABS_SETUP, &abs_y);

    struct uinput_setup setup = {
        .id = {
            .bustype = BUS_USB,
            .vendor  = 0x1234,
            .product = 0x5678,
            .version = 1,
        },
    };
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "AbsoluteTouchMouse");
    ioctl(fd, UI_DEV_SETUP, &setup);
    ioctl(fd, UI_DEV_CREATE);

    usleep(100000);  //give the device a moment to initialize
    uinput_fd = fd;
}

void AbsoluteTouchMouse::DestroyUInputDevice() {
    if (uinput_fd < 0) return;
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    uinput_fd = -1;
}


void AbsoluteTouchMouse::OpenTouchpad(const char *device_path) {
    if (pad_fd >= 0) CloseTouchpad();

    int touch_fd = open(device_path, O_RDONLY);
    if (touch_fd < 0) {
        perror(":( cannot open touchpad device");
    }
    pad_fd = touch_fd;
}

void AbsoluteTouchMouse::CloseTouchpad() {
    if (pad_fd < 0) return;

    close(pad_fd);
    pad_fd = -1;
}


void AbsoluteTouchMouse::ProcessEvent(struct input_event &ev) {
    if (uinput_fd < 0) {
        if (!enabled) {
            usleep(100);
            return;
        }
        printf("invalid uinput_fd, setting enabled to false\n");
        enabled = false;
        return;
    }

    if (ev.type == EV_ABS) {
        std::lock_guard<std::mutex> lg(positions_lock);
        if (ev.code == ABS_X) {
            absolute_x = ev.value;
            clamped_absolute_x = ev.value;
            x_updated = 1;
        }
        else if (ev.code == ABS_Y) {
            absolute_y = ev.value;
            clamped_absolute_y = ev.value;
            y_updated = 1;
        }
        else return;

        int screen_x, screen_y;
        if (enable_area) {
            //clamp coordinates to the defined region
            if (clamped_absolute_x < area_min_x) clamped_absolute_x = area_min_x;
            if (clamped_absolute_x > area_max_x) clamped_absolute_x = area_max_x;
            if (clamped_absolute_y < area_min_y) clamped_absolute_y = area_min_y;
            if (clamped_absolute_y > area_max_y) clamped_absolute_y = area_max_y;

            //map region to full screen
            screen_x = (absolute_x - area_min_x) * screen_width / (area_max_x - area_min_x);
            screen_y = (absolute_y - area_min_y) * screen_height / (area_max_y - area_min_y);
        }
        else {
            screen_x = absolute_x * screen_width / absolute_max_x;
            screen_y = absolute_y * screen_height / absolute_max_y;
        }

        //send cursor position from our fake device
        if (x_updated) emit(uinput_fd, EV_ABS, ABS_X, screen_x);
        if (y_updated) emit(uinput_fd, EV_ABS, ABS_Y, screen_y);
        send_syn(uinput_fd);
        if (verbose) printf("%i %i\n", screen_x, screen_y);
        pulling_rate_tmp += (x_updated || y_updated);
        x_updated = 0;
        y_updated = 0;
    }
    else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
        struct timeval now;
        gettimeofday(&now, nullptr);

        if (ev.value == 1) {
            gettimeofday(&last_down, nullptr);
            tap_active = 1;
        } else if (ev.value == 0 && tap_active) {
            long dt = (now.tv_sec - last_down.tv_sec) * 1000 +
                      (now.tv_usec - last_down.tv_usec) / 1000;

            if (dt < 200) { //200 ms tap (simulate a click)
                emit(uinput_fd, EV_KEY, BTN_LEFT, 1);
                send_syn(uinput_fd);
                usleep(10000);
                emit(uinput_fd, EV_KEY, BTN_LEFT, 0);
                send_syn(uinput_fd);
            }
            tap_active = 0;
        }
    }
}

void AbsoluteTouchMouse::Tick() {
    if (pad_fd < 0) {
        if (!enabled) {
            usleep(100); //this prevents the program from crashing somehow
            return;
        }
        enabled = false;
        printf("invalid pad_fd, enabled set to false\n");
        return;
    };

    struct input_event ev;

    //for poll()
    struct pollfd fds[1];
    fds[0].fd = pad_fd;
    fds[0].events = POLLIN;

    struct timeval now;
    gettimeofday(&now, nullptr);

    long dt_pr = (now.tv_sec - pulling_rate_start.tv_sec) * 1000 +
                 (now.tv_usec - pulling_rate_start.tv_usec) / 1000;

    if (dt_pr >= 1000) { //update pulling_rate every 1 s
        pulling_rate_start = now;
        pulling_rate = pulling_rate_tmp;
        pulling_rate_tmp = 0;
        if (show_pulling_rate) printf("pulling rate (EV_SYN/sec) %i\n", pulling_rate);
    }

    //read from actual trackpad
    if (poll(fds, 1, 1000) <= 0) return; //non-blocking way of checking if there are events comming from the trackpad
    ssize_t n = read(pad_fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) return;
    if (!enabled) return;

    ProcessEvent(ev);
}
