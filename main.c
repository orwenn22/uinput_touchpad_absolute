#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sched.h>


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
//now put whatever device your trackpad is in here
#define TOUCHPAD_DEV "/dev/input/event9"

//and put the touchpad's maximum absolute coordinates here
#define PAD_MAX_X 3220
#define PAD_MAX_Y 1952

//stuff to map a region of the trackpad to the screen ("tablet area") (only used if FULLSCREEN_MODE is not defined, else it will just use PAD_MAX_X and PAD_MAX_Y)
#define PAD_REGION_MIN_X 1000
#define PAD_REGION_MIN_Y 500
#define PAD_REGION_MAX_X (PAD_REGION_MIN_X+1500)
#define PAD_REGION_MAX_Y (PAD_REGION_MIN_Y+1000)

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//TODO: add ability to map a region of the screen?

//uncomment this if you want to use your entire screen (or for debugging purpose idk)
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

void emit(int fd, int type, int code, int val) {
    struct input_event ev = {0};
    ev.type = type;
    ev.code = code;
    ev.value = val;
    gettimeofday(&ev.time, NULL);
    write(fd, &ev, sizeof(ev));
}

void send_syn(int fd) {
    emit(fd, EV_SYN, SYN_REPORT, 0);
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

    struct input_event ev;
    int abs_x = 0, abs_y = 0;
    int touching = 0;
    int x_updated = 0, y_updated = 0;

    //used allow clicking by just tapping
    struct timeval last_down = {0};
    int tap_active = 0;

    while (1) {
        ssize_t n = read(touch_fd, &ev, sizeof(ev)); //read event from real trackpad
        if (n != sizeof(ev)) continue;

        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X) {
                abs_x = ev.value;
                x_updated = 1;
            }
            else if (ev.code == ABS_Y) {
                abs_y = ev.value;
                y_updated = 1;
            }

#ifndef FULLSCREEN_MODE
            //clamp coordinates to the defined region
            if (abs_x < PAD_REGION_MIN_X) abs_x = PAD_REGION_MIN_X;
            if (abs_x > PAD_REGION_MAX_X) abs_x = PAD_REGION_MAX_X;
            if (abs_y < PAD_REGION_MIN_Y) abs_y = PAD_REGION_MIN_Y;
            if (abs_y > PAD_REGION_MAX_Y) abs_y = PAD_REGION_MAX_Y;

            //map region to full screen
            int screen_x = (abs_x - PAD_REGION_MIN_X) * SCREEN_WIDTH / (PAD_REGION_MAX_X - PAD_REGION_MIN_X);
            int screen_y = (abs_y - PAD_REGION_MIN_Y) * SCREEN_HEIGHT / (PAD_REGION_MAX_Y - PAD_REGION_MIN_Y);
#else
            //fullscreen (old code)
            int screen_x = abs_x * SCREEN_WIDTH / PAD_MAX_X;
            int screen_y = abs_y * SCREEN_HEIGHT / PAD_MAX_Y;
#endif

            //send cursor position from our fake device
            if (x_updated && y_updated) { //somehow, adding that condition makes stuff more efficient
                emit(uinput_fd, EV_ABS, ABS_X, screen_x);
                emit(uinput_fd, EV_ABS, ABS_Y, screen_y);
                send_syn(uinput_fd);
                x_updated = 0;
                y_updated = 0;
            }
        }
        else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) {
                gettimeofday(&last_down, NULL);
                tap_active = 1;
            } else if (ev.value == 0 && tap_active) {
                struct timeval now;
                gettimeofday(&now, NULL);
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

    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(touch_fd);
    return 0;
}
