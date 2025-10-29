#include "SecondaryThread.h"

SecondaryThread::SecondaryThread(int id) : m_id(id), m_main_thread(nullptr), m_running(false) { }

SecondaryThread::~SecondaryThread() = default;

void SecondaryThread::InternalRun() {
    m_running = true;
    Run();
    m_running = false;
}
