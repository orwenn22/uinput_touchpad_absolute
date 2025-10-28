cmake_minimum_required(VERSION 4.0)
project(uinput_touchpad_absolute CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(uinput_touchpad_absolute main.cpp
        main.h
        utils.h
        console_input_thread.cpp
        console_input_thread.h
        AbsoluteTouchMouse.cpp
        AbsoluteTouchMouse.h)
