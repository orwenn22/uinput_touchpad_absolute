#ifndef UINPUT_TOUCHPAD_ABSOLUTE_UTILS_H
#define UINPUT_TOUCHPAD_ABSOLUTE_UTILS_H

#include <unistd.h>
#include <ctime>
#include <linux/input.h>

inline void emit(int fd, int type, int code, int val) {
    struct input_event ev = {0};
    ev.type = type;
    ev.code = code;
    ev.value = val;
    gettimeofday(&ev.time, nullptr);
    write(fd, &ev, sizeof(ev));
}

inline void send_syn(int fd) {
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

#endif //UINPUT_TOUCHPAD_ABSOLUTE_UTILS_H