#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sched.h>

#include <thread>
#include <vector>

#include "console_input_thread.h"
#include "utils.h"


//how 2 find your trackpad :)
//run this
// sudo libinput list-devices
//
//look for anything that looks like a trackpad, and try running this on them
// sudo evtest /dev/input/event<some_number>
//
//if at the top you have something like this
// Event code 0 (ABS_X) Min 0 Max 3220
// Event code 1 (ABS_Y) Min 0 Max 1952
//congrats, your touchpad will most likely work! :D
//(make sure there are also absolute events when you move your finger on your trackpad)
//
//now put whatever device your trackpad is in here (NOTE: THIS MAY CHANGE ON REBOOT)
#define TOUCHPAD_DEV "/dev/input/event8"

//and put the touchpad's maximum absolute coordinates here
#define PAD_MAX_X 3220
#define PAD_MAX_Y 1952

//stuff to map a region of the trackpad to the screen ("tablet area") (only used if FULLSCREEN_MODE is not defined, else it will just use PAD_MAX_X and PAD_MAX_Y)
#define PAD_REGION_MIN_X 1000
#define PAD_REGION_MIN_Y 500
#define PAD_REGION_MAX_X (PAD_REGION_MIN_X+1500)
#define PAD_REGION_MAX_Y (PAD_REGION_MIN_Y+1000)

//screen resolution, multi-monitor setups are (probably?) not supported
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//TODO: add ability to map a region of the screen?

//uncomment this if you want to map your entire screen
#define FULLSCREEN_MODE

//OPTIONAL
//
//you might want to disable your trackpad temporarely while running this
//THIS IS PERSISTANT AND WILL DISABLE YOUR TRACKPAD UNTIL YOU ENABLE IT BACK,
//USE AT YOUR OWN RISK
//gnome: gsettings set org.gnome.desktop.peripherals.touchpad send-events 'disabled'
//gnome (re-enable): gsettings reset org.gnome.desktop.peripherals.touchpad send-events
//sway (hyprland maybe?): swaymsg input "type:touchpad" events disabled
//sway (re-enable): swaymsg input "type:touchpad" events endabled


MainThreadInfo g_main_thread_info = {0};

int setup_uinput_device() {
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
    return fd;
}


int main(void) {
    int touch_fd = open(TOUCHPAD_DEV, O_RDONLY);
    if (touch_fd < 0) {
        perror(":( cannot open touchpad device");
        return 1;
    }

    int uinput_fd = setup_uinput_device();

    printf("reading absolute coordinates from %s\n", TOUCHPAD_DEV);
    printf("moving virtual cursor via uinput... (ctrl+c to quit)\n");

    //bigger thread priority for less input latency? idk
    struct sched_param param;
    param.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("Failed to set real-time priority");
    }

    g_main_thread_info.absolute_x = 0;
    g_main_thread_info.absolute_y = 0;
    g_main_thread_info.running = true;
    g_main_thread_info.last_down = {0};
    g_main_thread_info.tap_active = 0;
    gettimeofday(&g_main_thread_info.pulling_rate_start, nullptr);
    g_main_thread_info.pulling_rate = 0;
    g_main_thread_info.pulling_rate_tmp = 0;

    std::vector<std::thread> threads;
    threads.emplace_back(console_input_thread_main);

    struct input_event ev;
    int touching = 0;
    int x_updated = 0, y_updated = 0;

    //for poll()
    struct pollfd fds[1];
    fds[0].fd = touch_fd;
    fds[0].events = POLLIN;

    while (g_main_thread_info.running) {
        //monitor pulling rate
        struct timeval now;
        gettimeofday(&now, NULL);

        long dt_pr = (now.tv_sec - g_main_thread_info.pulling_rate_start.tv_sec) * 1000 +
                     (now.tv_usec - g_main_thread_info.pulling_rate_start.tv_usec) / 1000;

        if (dt_pr >= 1000) { //update pulling_rate every 1 s
            g_main_thread_info.pulling_rate_start = now;
            g_main_thread_info.pulling_rate = g_main_thread_info.pulling_rate_tmp;
            g_main_thread_info.pulling_rate_tmp = 0;
            printf("pulling rate (EV_SYN/sec) %i\n", g_main_thread_info.pulling_rate);
        }

        //read from actual trackpad
        if (poll(fds, 1, 1000) <= 0) continue; //non-blocking way of checking if there are events comming from the trackpad
        ssize_t n = read(touch_fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) continue;

        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X) {
                g_main_thread_info.absolute_x = ev.value;
                x_updated = 1;
            }
            else if (ev.code == ABS_Y) {
                g_main_thread_info.absolute_y = ev.value;
                y_updated = 1;
            }

#ifndef FULLSCREEN_MODE
            //clamp coordinates to the defined region
            if (g_main_thread_info.absolute_x < PAD_REGION_MIN_X) g_main_thread_info.absolute_x = PAD_REGION_MIN_X;
            if (g_main_thread_info.absolute_x > PAD_REGION_MAX_X) g_main_thread_info.absolute_x = PAD_REGION_MAX_X;
            if (g_main_thread_info.absolute_y < PAD_REGION_MIN_Y) g_main_thread_info.absolute_y = PAD_REGION_MIN_Y;
            if (g_main_thread_info.absolute_y > PAD_REGION_MAX_Y) g_main_thread_info.absolute_y = PAD_REGION_MAX_Y;

            //map region to full screen
            int screen_x = (g_main_thread_info.absolute_x - PAD_REGION_MIN_X) * SCREEN_WIDTH / (PAD_REGION_MAX_X - PAD_REGION_MIN_X);
            int screen_y = (g_main_thread_info.absolute_y - PAD_REGION_MIN_Y) * SCREEN_HEIGHT / (PAD_REGION_MAX_Y - PAD_REGION_MIN_Y);
#else
            //fullscreen (old code)
            int screen_x = g_main_thread_info.absolute_x * SCREEN_WIDTH / PAD_MAX_X;
            int screen_y = g_main_thread_info.absolute_y * SCREEN_HEIGHT / PAD_MAX_Y;
#endif

            //send cursor position from our fake device
            if (x_updated) emit(uinput_fd, EV_ABS, ABS_X, screen_x);
            if (y_updated) emit(uinput_fd, EV_ABS, ABS_Y, screen_y);
            send_syn(uinput_fd);
            x_updated = 0;
            y_updated = 0;
            ++g_main_thread_info.pulling_rate_tmp;
        }
        else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) {
                gettimeofday(&g_main_thread_info.last_down, NULL);
                g_main_thread_info.tap_active = 1;
            } else if (ev.value == 0 && g_main_thread_info.tap_active) {
                long dt = (now.tv_sec - g_main_thread_info.last_down.tv_sec) * 1000 +
                          (now.tv_usec - g_main_thread_info.last_down.tv_usec) / 1000;
                if (dt < 200) { //200 ms tap (simulate a click)
                    emit(uinput_fd, EV_KEY, BTN_LEFT, 1);
                    send_syn(uinput_fd);
                    usleep(10000);
                    emit(uinput_fd, EV_KEY, BTN_LEFT, 0);
                    send_syn(uinput_fd);
                }
                g_main_thread_info.tap_active = 0;
            }
        }
    }

    printf("joining threads\n");
    g_main_thread_info.running = false; //in case the loop is ended with a break somehow (shouldn't happen)
    for (auto& thread : threads) thread.join();

    printf("destroying fds\n");
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(touch_fd);

    printf("exited cleanly\n");
    return 0;
}
