#include "main.h"

#include <cstdio>
#include <sched.h>

#include <thread>
#include <vector>

#include "AbsoluteTouchMouse.h"
#include "console_input_thread.h"

AbsoluteTouchMouse g_main_thread_device = AbsoluteTouchMouse();

int main() {
    g_main_thread_device.OpenTouchpad(TOUCHPAD_DEV);
    g_main_thread_device.CreateUInputDevice();

    printf("reading absolute coordinates from %s\n", TOUCHPAD_DEV);
    printf("moving virtual cursor via uinput... (ctrl+c to quit)\n");

    //bigger thread priority for less input latency? idk
    sched_param param{};
    param.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("Failed to set real-time priority");
    }

    std::vector<std::thread> threads;
    threads.emplace_back(console_input_thread_main);

    while (g_main_thread_device.running) {
        g_main_thread_device.Tick();
    }

    printf("joining threads\n");
    g_main_thread_device.running = false; //in case the loop is ended with a break somehow (shouldn't happen)
    for (auto& thread : threads) thread.join();

    printf("destroying fds\n");
    g_main_thread_device.DestroyUInputDevice();
    g_main_thread_device.CloseTouchpad();

    printf("exited cleanly\n");
    return 0;
}
