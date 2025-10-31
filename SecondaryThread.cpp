#include "SecondaryThread.h"

#include <cstdio>
#include <mutex>

SecondaryThread::SecondaryThread(int id) : m_id(id), m_main_thread(nullptr), m_running(false) { }

SecondaryThread::~SecondaryThread() {
    // signal thread to stop
    m_running = false;

    // join if joinable and not joining ourselves
    std::lock_guard<std::mutex> lg(m_thread_mutex);
    if (m_thread.joinable()) {
        if (m_thread.get_id() != std::this_thread::get_id()) {
            m_thread.join();
            printf("~SecondaryThread : joined\n");
        } else {
            //if this object is being destroyed from inside the worker thread,
            //don't join (joining self would deadlock). Prefer that destruction
            //be done by the owning thread instead — but this guard prevents UB.
            //this might also get reached if the tread is deleted without being started
        }
    }
}

void SecondaryThread::InternalRun() {
    m_running = true;
    Run();
    m_running = false;
}
