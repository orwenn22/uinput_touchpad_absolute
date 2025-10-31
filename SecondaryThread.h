#ifndef UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H
#define UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H
#include <atomic>
#include <mutex>
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

    //meant to be called by Run() to stop the stread
    void Stop() { m_running = false; }

    //TODO: maybe this should return m_running && m_main_thread->Running()?
    bool Running() { return m_running; }

private:
    friend MainThread;
    MainThread *m_main_thread{};
    int m_id;

    std::thread m_thread;
    std::mutex m_thread_mutex;

    //this will be set to false by the main thread whenever this thread needs to stop, but the new thread can also set
    //this to false by calling Stop(), and the main thread will join the thread after that.
    //this is also automatically set to false whenever Run() returns.
    std::atomic<bool> m_running;

    void InternalRun();
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_SECONDARYTHREAD_H