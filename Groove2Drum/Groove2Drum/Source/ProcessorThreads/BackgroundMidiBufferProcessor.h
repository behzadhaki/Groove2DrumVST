//
// Created by Behzad Haki on 2022-08-04.
//

#ifndef JUCECMAKEREPO_BACKGROUNDMIDIBUFFERPROCESSOR_H
#define JUCECMAKEREPO_BACKGROUNDMIDIBUFFERPROCESSOR_H

#include "../PluginProcessor.h"
#include "../settings.h"

class BackgroundMidiBufferProcessor: public juce::Thread
{
    BackgroundMidiBufferProcessor();

    ~BackgroundMidiBufferProcessor() override;

    // Override this with the task to be done on received data from queue
    // See examples of the child classes below
    virtual void QueueDataProcessor();

    // No need to override in the children classes
    void run() override;
};

#endif //JUCECMAKEREPO_BACKGROUNDMIDIBUFFERPROCESSOR_H