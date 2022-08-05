//
// Created by Behzad Haki on 2022-08-05.
//

#ifndef JUCECMAKEREPO_GROOVETHREAD_H
#define JUCECMAKEREPO_GROOVETHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>

class GrooveThread:public juce::Thread
{
public:

    GrooveThread();
    // ~GrooveThread() override;

    void prepareToStop();

    void run() override;

};

#endif //JUCECMAKEREPO_GROOVETHREAD_H
