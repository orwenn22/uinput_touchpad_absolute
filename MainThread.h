#ifndef UINPUT_TOUCHPAD_ABSOLUTE_MAINTHREAD_H
#define UINPUT_TOUCHPAD_ABSOLUTE_MAINTHREAD_H
#include <thread>
#include <vector>

#include "AbsoluteTouchMouse.h"


class SecondaryThread;
struct AbsoluteTouchMouse;

class MainThread {
public:
    MainThread();
    virtual ~MainThread();

    void RunLoop();

    void AddThread(SecondaryThread *thread);
    bool IsThreadRunning(int id);

    AbsoluteTouchMouse *GetAbsoluteTouchMouse() { return &m_device; }

    std::atomic<bool> m_running;

private:
    std::vector<SecondaryThread *> m_threads;
    AbsoluteTouchMouse m_device;
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_MAINTHREAD_H