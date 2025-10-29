#include "main.h"

#include <cstdio>
#include "ConsoleInputThread.h"
#include "MainThread.h"


int main() {
    //bigger thread priority for less input latency? idk
    sched_param param{};
    param.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("Failed to set real-time priority");
    }

    {
        MainThread main_thread;
        main_thread.AddThread(new ConsoleInputThread());
        main_thread.RunLoop();
    } //MainThread is destroyed here

    printf("exited cleanly\n");
    return 0;
}
