#ifndef UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H
#define UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H

#include <atomic>
#include <ctime>

struct MainThreadInfo {
    int absolute_x, absolute_y;
    std::atomic<bool> running;

    struct timeval last_down = {0};
    int tap_active;

    struct timeval pulling_rate_start = {0};
    int pulling_rate;
    int pulling_rate_tmp;
};

extern MainThreadInfo g_main_thread_info;

#endif //UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H