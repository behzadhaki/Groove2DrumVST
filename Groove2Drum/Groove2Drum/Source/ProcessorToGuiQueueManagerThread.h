//
// Created by Behzad Haki on 2022-08-23.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "InterThreadFifos.h"
#include "gui/PianoRoll_GeneratedDrums.h"

class ProcessorToGuiQueueManagerThread: public juce::Thread
{
public:
    ProcessorToGuiQueueManagerThread():Thread("ProcessorToGuiQueueManagerThread")
    {

    }

    void startThreadUsingProvidedResources(PianoRoll_GeneratedDrums_AllVoices* DrumsPianoRollWidgetPntr,
                                           GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues* ModelThreadToDrumPianoRollWidgetQuesPntr)
    {
        // Resources required for updating the generated drums widget
        DrumsPianoRollWidget = DrumsPianoRollWidgetPntr;
        ModelThreadToDrumPianoRollWidgetQues = ModelThreadToDrumPianoRollWidgetQuesPntr;

        // start thread
        startThread();
    }

    void QueueDataProcessor()
    {
        // process queue data here
        if (DrumsPianoRollWidget != nullptr and ModelThreadToDrumPianoRollWidgetQues != nullptr)
        {
            if (ModelThreadToDrumPianoRollWidgetQues->new_generated_data.getNumReady() > 0)
            {
                auto latest_generated_data = ModelThreadToDrumPianoRollWidgetQues->new_generated_data.getLatestOnly();
                juce::MessageManagerLock mmlock;

                for (int t_= 0; t_ < latest_generated_data.time_steps; t_++)
                {
                    for (int vn_= 0; vn_ < latest_generated_data.num_voices; vn_++)
                    {

                        DBG(" ADDING STEP "<<t_<<" ,voice "<<vn_<< " ppq "<< latest_generated_data.ppqs[t_][vn_].item().toFloat());
                        DBG(" hit "<<latest_generated_data.hits[t_][vn_].item().toInt()<<" ,vel "<<latest_generated_data.velocities[t_][vn_].item().toFloat()<< " prob "<< latest_generated_data.hit_probabilities[t_][vn_].item().toFloat());

                        DrumsPianoRollWidget->addEventWithPPQ(
                            vn_,
                            latest_generated_data.ppqs[t_][vn_].item().toFloat(),
                            latest_generated_data.hits[t_][vn_].item().toInt(),
                            latest_generated_data.velocities[t_][vn_].item().toFloat(),
                            latest_generated_data.hit_probabilities[t_][vn_].item().toFloat());
                    }
                }
            }
        }
    }

    void run () override
    {
        bool bExit = threadShouldExit();
        while (!bExit)
        {
            QueueDataProcessor();
            bExit = threadShouldExit();
            sleep (10); // avoid burning CPU, if reading is returning immediately
        }
    }

private:
    // queues from which data is received
    GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues* ModelThreadToDrumPianoRollWidgetQues;

    // widgets which should updated based on received data
    PianoRoll_GeneratedDrums_AllVoices* DrumsPianoRollWidget;
};
