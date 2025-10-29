#ifndef UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H
#define UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H
#include <atomic>
#include <thread>


class MainThread;

class SecondaryThread {
public:
    SecondaryThread(int id);
    virtual ~SecondaryThread();

    int GetID() const { return m_id; }

    virtual void Run() = 0;

protected:
    MainThread *GetMainThread() const { return m_main_thread; }
    std::atomic<bool> m_running; //this will be set to false by the main thread whenever this thread needs to stop, but the new thread can also set this to false to make the main thread delete it

private:
    friend MainThread;
    MainThread *m_main_thread{};
    int m_id;

    std::thread m_thread;

    void InternalRun();
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H