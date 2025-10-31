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
    void StartSecondaryThread(SecondaryThread *thread);

    std::vector<SecondaryThread *> m_threads;
    std::vector<SecondaryThread *> m_new_threads; //if some threads are added before RunLoop is called, they end up here
    std::mutex m_threads_mutex;
    AbsoluteTouchMouse m_device;
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_MAINTHREAD_H