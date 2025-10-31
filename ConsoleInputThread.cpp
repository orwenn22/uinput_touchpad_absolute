#include "ConsoleInputThread.h"

#include <cstdio>
#include <iostream>
#include <string>

#include "MainThread.h"
#include "Visualizer/VisualizerThread.h"

ConsoleInputThread::ConsoleInputThread() : SecondaryThread(1) {}

ConsoleInputThread::~ConsoleInputThread() = default;

void ConsoleInputThread::Run() {
    auto device = GetMainThread()->GetAbsoluteTouchMouse();
    while (GetMainThread()->m_running && Running()) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line == "area") ToggleArea(device);
        else if (line == "verbose") ToggleVerbose(device);
        else if (line == "p") TogglePullingRate(device);
        else if (line == "q" || line == "quit") break;
        else if (line == "enable") device->enabled = true;
        else if (line == "disable") device->enabled = false;
        else if (line == "v") LaunchVisualizer();
        else printf("unknown command '%s'\n", line.c_str());
    }
    GetMainThread()->m_running = false; //quit entire application
    printf("ConsoleInputThread end\n");
}

void ConsoleInputThread::ToggleArea(AbsoluteTouchMouse *device) {
    if (!device->enable_area) { //area currently disabled
        printf("enabled area (min [%i %i], max [%i %i])\n",
            device->area_min_x, device->area_min_y,
            device->area_max_x, device->area_max_y);
    }
    else { //area currently enabled
        printf("disabled area\n");
    }

    device->enable_area = !device->enable_area;
}

void ConsoleInputThread::ToggleVerbose(AbsoluteTouchMouse *device) {
    device->verbose = !device->verbose;
}

void ConsoleInputThread::TogglePullingRate(AbsoluteTouchMouse *device) {
    device->show_pulling_rate = !device->show_pulling_rate;
}

void ConsoleInputThread::LaunchVisualizer() {
    GetMainThread()->AddThread(new VisualizerThread);
}

