#ifndef UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H
#define UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H


//how 2 find your trackpad :)
//run this
// sudo libinput list-devices
//
//look for anything that looks like a trackpad, and try running this on them
// sudo evtest /dev/input/event<some_number>
//
//if at the top you have something like this
// Event code 0 (ABS_X) Min 0 Max 3220
// Event code 1 (ABS_Y) Min 0 Max 1952
//congrats, your touchpad will most likely work! :D
//(make sure there are also absolute events when you move your finger on your trackpad)
//
//now put whatever device your trackpad is in here (NOTE: THIS MAY CHANGE ON REBOOT)
#define TOUCHPAD_DEV "/dev/input/event9"

//and put the touchpad's maximum absolute coordinates here
#define PAD_MAX_X 3220
#define PAD_MAX_Y 1952

//stuff to map a region of the trackpad to the screen ("tablet area") (only used if FULLSCREEN_MODE is not defined, else it will just use PAD_MAX_X and PAD_MAX_Y)
#define PAD_REGION_MIN_X 1000
#define PAD_REGION_MIN_Y 500
#define PAD_REGION_MAX_X (PAD_REGION_MIN_X+1500)
#define PAD_REGION_MAX_Y (PAD_REGION_MIN_Y+1000)

//screen resolution
//technically this could be any arbitrary value as long as it's big enough, but it's better to put it the same size as your monitor
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//TODO: add ability to map a region of the screen?
//TODO 2: load all of this from a config file?

//OPTIONAL
//
//you might want to disable your trackpad temporarely while running this
//THIS IS PERSISTANT AND WILL DISABLE YOUR TRACKPAD UNTIL YOU ENABLE IT BACK,
//USE AT YOUR OWN RISK
//gnome: gsettings set org.gnome.desktop.peripherals.touchpad send-events 'disabled'
//gnome (re-enable): gsettings reset org.gnome.desktop.peripherals.touchpad send-events
//sway (hyprland maybe?): swaymsg input "type:touchpad" events disabled
//sway (re-enable): swaymsg input "type:touchpad" events endabled


#include "AbsoluteTouchMouse.h"

extern AbsoluteTouchMouse g_main_thread_device;

#endif //UINPUT_TOUCHPAD_ABSOLUTE_MAIN_H