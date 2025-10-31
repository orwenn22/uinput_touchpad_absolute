#ifndef UINPUT_TOUCHPAD_ABSOLUTE_VISUALIZERTHREAD_H
#define UINPUT_TOUCHPAD_ABSOLUTE_VISUALIZERTHREAD_H
#include "../SecondaryThread.h"


class VisualizerThread : public SecondaryThread {
public:
    VisualizerThread();
    ~VisualizerThread() override;

    void Run() override;


private:
    static std::atomic<bool> s_already_running; //there should only be one instance of the visualizer thread
};


#endif //UINPUT_TOUCHPAD_ABSOLUTE_VISUALIZERTHREAD_H