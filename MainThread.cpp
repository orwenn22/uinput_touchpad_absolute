#include "MainThread.h"

#include <cstdio>

#include "main.h"
#include "SecondaryThread.h"

MainThread::MainThread() : m_running(false), m_device() { }

MainThread::~MainThread() {
    m_threads_mutex.lock();
    for (auto &thread : m_threads) {
        thread->m_running = false;
        printf("~MainThread : joining thread\n");
        delete thread;
    }
    m_threads.clear();

    for (auto &thread : m_new_threads) {
        thread->m_running = false;
        delete thread;
    }
    m_threads_mutex.unlock();
}

void MainThread::RunLoop() {
    m_running = true;

    m_device.OpenTouchpad(TOUCHPAD_DEV);
    m_device.CreateUInputDevice();

    printf("reading absolute coordinates from %s\n", TOUCHPAD_DEV);
    printf("moving virtual cursor via uinput... (q to quit)\n");

    for (auto &thread : m_new_threads) StartSecondaryThread(thread);
    m_new_threads.clear();

    while (m_running) {
        m_device.Tick();

        //this kinda sucks, because if some random thread sets its m_running value to false, but still sets its m_running to false,
        //it will block the main tread. maybe calling m_device.Tick() should be moved in its own secondary thread?
        //this shouldn't be an issue for the time being, since the user will most likely never stop a secondary thread while
        //doing something that is sensitive to input-latency
        m_threads_mutex.lock();
        for (int i = 0; i < m_threads.size(); i++) {
            if (m_threads[i]->m_running) continue;
            printf("joining thread (RunLoop loop)\n");
            delete m_threads[i];
            m_threads.erase(m_threads.begin() + i);
            --i;
        }
        m_threads_mutex.unlock();
    }

    printf("joining threads\n");
    m_running = false; //in case the loop is ended with a break somehow (shouldn't happen)

    m_threads_mutex.lock();
    for (auto &thread : m_threads) {
        printf("joining thread (RunLoop end)\n");
        thread->m_running = false;
        delete thread;
    }
    m_threads.clear();
    m_threads_mutex.unlock();

    printf("destroying fds\n");
    m_device.DestroyUInputDevice();
    m_device.CloseTouchpad();
}

void MainThread::AddThread(SecondaryThread *thread) {
    if (thread == nullptr) return;
    if (thread->m_running) return;

    // set ownership + init fields before starting
    thread->m_main_thread = this;

    if (!m_running) {
        m_new_threads.push_back(thread);
        return;
    }

    StartSecondaryThread(thread);
}


bool MainThread::IsThreadRunning(int id) {
    std::lock_guard<std::mutex> lg(m_threads_mutex);

    for (const auto &thread : m_threads) {
        if (thread->GetID() == id) {
            return true;
        }
    }
    return false;
}


void MainThread::StartSecondaryThread(SecondaryThread *thread) {
    if (thread == nullptr) return;

    //store in threads
    {
        std::lock_guard<std::mutex> lg(m_threads_mutex);
        m_threads.push_back(thread);
    }

    //actuallu start the thread
    {
        std::lock_guard<std::mutex> lg(thread->m_thread_mutex);

        //we need to set this to true now because we don't exactly know when InternalRun will be called.
        //maybe by then we will already be back in the RunLoop, and it will try to delete the thread if Internal hasn't been called yet
        thread->m_running = true;
        thread->m_thread = std::thread(&SecondaryThread::InternalRun, thread);
    }
}