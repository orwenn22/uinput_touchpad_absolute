#include "main.h"

#include <iostream>
#include <string>

void toggle_area() {
    if (!g_main_thread_info.enable_area) { //area currently disabled
        printf("enabled area (min [%i %i], max [%i %i])\n",
            g_main_thread_info.area_min_x, g_main_thread_info.area_min_y,
            g_main_thread_info.area_max_x, g_main_thread_info.area_max_y);
    }
    else { //area currently enabled
        printf("disabled area\n");
    }

    g_main_thread_info.enable_area = !g_main_thread_info.enable_area;
}

void toggle_pulling_rate() {
    g_main_thread_info.show_pulling_rate = !g_main_thread_info.show_pulling_rate;
}

void toggle_verbose() {
    g_main_thread_info.verbose = !g_main_thread_info.verbose;
}

void console_input_thread_main() {
    while (g_main_thread_info.running) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line == "area") toggle_area();
        else if (line == "verbose") toggle_verbose();
        else if (line == "p") toggle_pulling_rate();
        else if (line == "q" || line == "quit") g_main_thread_info.running = false;
        else if (line == "enable") g_main_thread_info.enabled = true;
        else if (line == "disable") g_main_thread_info.enabled = false;
        else printf("unknown command '%s'\n", line.c_str());
    }
    g_main_thread_info.running = false; //in case getline fails
}