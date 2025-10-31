#ifndef UINPUT_TOUCHPAD_ABSOLUTE_CONSOLEINPUTTHREAD_H
#define UINPUT_TOUCHPAD_ABSOLUTE_CONSOLEINPUTTHREAD_H
#include "SecondaryThread.h"


struct AbsoluteTouchMouse;

class ConsoleInputThread : public SecondaryThread {
public:
    ConsoleInputThread();
    ~ConsoleInputThread() override;

    void Run() override;

private:
    void ToggleArea(AbsoluteTouchMouse *device);
    void ToggleVerbose(AbsoluteTouchMouse *device);
    void TogglePullingRate(AbsoluteTouchMouse *device);
    void LaunchVisualizer();
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_CONSOLEINPUTTHREAD_H