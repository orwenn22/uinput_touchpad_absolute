cmake_minimum_required(VERSION 4.0)
project(uinput_touchpad_absolute CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(raylib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fno-inline -g -O0")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=thread")

add_executable(uinput_touchpad_absolute main.cpp
        main.h
        utils.h
        AbsoluteTouchMouse.cpp
        AbsoluteTouchMouse.h
        MainThread.cpp
        MainThread.h
        SecondaryThread.cpp
        SecondaryThread.h
        ConsoleInputThread.cpp
        ConsoleInputThread.h
        Visualizer/VisualizerThread.cpp
        Visualizer/VisualizerThread.h)

target_link_libraries(uinput_touchpad_absolute PRIVATE raylib)
