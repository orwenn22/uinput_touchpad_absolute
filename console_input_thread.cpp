#include "main.h"

#include <iostream>
#include <string>

void console_input_thread_main() {
    while (g_main_thread_info.running) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line[0] == 'q') g_main_thread_info.running = false;
    }
    g_main_thread_info.running = false; //in case getline fails
}