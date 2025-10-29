#include "MainThread.h"

#include <cstdio>

#include "main.h"
#include "SecondaryThread.h"

MainThread::MainThread() : m_running(true), m_device() { }

MainThread::~MainThread() {
    for (auto &thread : m_threads) {
        thread->m_running = false;
        printf("joining thread with id %i (destructor)\n", thread->m_id);
        thread->m_thread.join();
        delete thread;
    }
    m_threads.clear();
}

void MainThread::RunLoop() {
    m_device.OpenTouchpad(TOUCHPAD_DEV);
    m_device.CreateUInputDevice();

    printf("reading absolute coordinates from %s\n", TOUCHPAD_DEV);
    printf("moving virtual cursor via uinput... (ctrl+c to quit)\n");

    while (m_running) {
        m_device.Tick();

        //this kinda sucks, because if some random thread sets its m_running value to false, but still sets its m_running to false,
        //it will block the main tread. maybe calling m_device.Tick() should be moved in its own secondary thread?
        //this shouldn't be an issue for the time being, since the user will most likely never stop a secondary thread while
        //doing something that is sensitive to input-latency
        for (int i = 0; i < m_threads.size(); i++) {
            if (m_threads[i]->m_running) continue;
            printf("joining thread with id %i (m_running false)\n", m_threads[i]->m_id);
            m_threads[i]->m_thread.join();
            delete m_threads[i];
            m_threads.erase(m_threads.begin() + i);
        }
    }

    printf("joining threads\n");
    m_running = false; //in case the loop is ended with a break somehow (shouldn't happen)

    for (auto &thread : m_threads) {
        printf("joining thread with id %i (end of loop)\n", thread->m_id);
        thread->m_running = false;
        thread->m_thread.join();
        delete thread;
    }
    m_threads.clear();

    printf("destroying fds\n");
    m_device.DestroyUInputDevice();
    m_device.CloseTouchpad();
}

void MainThread::AddThread(SecondaryThread *thread) {
    if (thread == nullptr) return;
    if (thread->m_running) return;

    thread->m_main_thread = this;
    m_threads.push_back(thread);
    thread->m_thread = std::thread([=]() {thread->InternalRun();});
}

bool MainThread::IsThreadRunning(int id) {
    for (const auto &thread : m_threads) {
        if (thread->GetID() == id) return true;
    }
    return false;
}
