cmake_minimum_required(VERSION 4.0)
project(uinput_touchpad_absolute CXX)

set(CMAKE_CXX_STANDARD 17)

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
        ConsoleInputThread.h)
