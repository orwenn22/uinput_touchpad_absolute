#ifndef UINPUT_TOUCHPAD_ABSOLUTE_ABSOLUTETOUCHMOUSE_H
#define UINPUT_TOUCHPAD_ABSOLUTE_ABSOLUTETOUCHMOUSE_H

#include <atomic>
#include <ctime>
#include <mutex>

struct AbsoluteTouchMouse {
    AbsoluteTouchMouse();
    ~AbsoluteTouchMouse();

    void CreateUInputDevice();
    void DestroyUInputDevice();

    void OpenTouchpad(const char *device_path);
    void CloseTouchpad();

    //process an event from another device fd (actual trackpad events)
    void ProcessEvent(struct input_event &ev);

    //read events from pad_fd (actual trackpad), and if enabled is set to true, send them to ProcessEvent.
    //should be called in a while(1) loop
    void Tick();

    int uinput_fd;
    int pad_fd;

    std::atomic<bool> enabled;
    std::atomic<bool> verbose;

    int x_updated, y_updated;
    int absolute_max_x, absolute_max_y; //the pad's maximum absolute coordinates
    int absolute_x, absolute_y; //the finger's current absolute position on the trackpad
    int clamped_absolute_x, clamped_absolute_y; //the position restricted to the area if enable_area is true
    std::mutex positions_lock;

    int screen_width, screen_height;

    std::atomic<bool> enable_area;
    int area_min_x, area_min_y,
        area_max_x, area_max_y;

    struct timeval last_down = {0};
    int tap_active;

    std::atomic<bool> show_pulling_rate;
    struct timeval pulling_rate_start = {0};
    int pulling_rate; //number of SYN event sent from virtual device in the previous second
    int pulling_rate_tmp; //number of SYN event sent from the virtual device since pulling_rate_start
};

#endif //UINPUT_TOUCHPAD_ABSOLUTE_ABSOLUTETOUCHMOUSE_H