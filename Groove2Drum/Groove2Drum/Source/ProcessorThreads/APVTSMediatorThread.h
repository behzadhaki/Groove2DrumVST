//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once



#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../settings.h"
#include "../Includes/CustomStructsAndLockFreeQueue.h"

// ============================================================================================================
// ==========         This Thread is in charge of checking which parameters in APVTS have been changed.
// ==========           If changed, the updated value will be pushed to a corresponding queue to be read
// ==========           by the receiving/destination thread.
// ==========
// ==========         To read from APVTS, we always get a std::atomic pointer, which potentially
// ==========           can block a thread if some read/write race is happening. As a result, it is not
// ==========           safe to directly read from APVTS inside the processBlock() thread or any other
// ==========           time sensitive threads
// ==========
// ==========         In this plugin, the GrooveThread and the ModelThread are not time-sensitive. So,
// ==========           they can read from APVTS directly. Regardless, in future iterations, perhaps
// ==========           these requirements change. To be future-proof, this thread has been implemented
// ==========           to take care of mediating the communication of parameters in the APVTS to the
// ==========           ProcessorThreads as well as the processBlock()
// ============================================================================================================
class APVTSMediatorThread: public juce::Thread
{
public:

    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread(): juce::Thread("APVTSMediatorThread")
    {
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        juce::AudioProcessorValueTreeState* APVTSPntr,
        LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_QuePntr,
        LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_QuePntr
        )
    {
        APVTS = APVTSPntr;
        APVTS2GrooveThread_groove_vel_offset_ranges_Que = APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr;
        APVTS2ModelThread_max_num_hits_Que = APVTS2ModelThread_max_num_hits_QuePntr;
        APVTS2ModelThread_sampling_thresholds_Que = APVTS2ModelThread_sampling_thresholds_QuePntr;
        APVTS2ModelThread_midi_mappings_Que = APVTS2ModelThread_midi_mappings_QuePntr;

        startThread();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override
    {
        // notify if the thread is still running
        bool bExit = threadShouldExit();

        auto current_groove_vel_offset_ranges = get_groove_vel_offset_ranges();
        auto current_max_num_hits = get_max_num_hits();
        auto current_sampling_thresholds = get_sampling_thresholds();
        auto current_per_voice_midi_numbers = get_per_voice_midi_numbers();
        while (!bExit)
        {
            auto new_groove_vel_offset_ranges = get_groove_vel_offset_ranges();
            // check if vel/offset values changed
            if (current_groove_vel_offset_ranges != new_groove_vel_offset_ranges)
            {
                current_groove_vel_offset_ranges = new_groove_vel_offset_ranges;
                APVTS2GrooveThread_groove_vel_offset_ranges_Que->push(
                    new_groove_vel_offset_ranges);
            }

            // check if per voice allowed maximum number of hits has changed
            auto new_max_num_hits = get_max_num_hits();
            if (current_max_num_hits != new_max_num_hits)
            {
                current_max_num_hits = new_max_num_hits;
                APVTS2ModelThread_max_num_hits_Que->push(new_max_num_hits);
            }

            // check if per voice allowed sampling thresholds have changed
            auto new_sampling_thresholds =  get_sampling_thresholds();
            if (current_sampling_thresholds != new_sampling_thresholds)
            {
                current_sampling_thresholds = new_sampling_thresholds;
                APVTS2ModelThread_sampling_thresholds_Que->push(new_sampling_thresholds);
            }

            // check if per voice midi numbers have changed
            auto new_per_voice_midi_numbers = get_per_voice_midi_numbers();
            if (current_per_voice_midi_numbers != get_per_voice_midi_numbers())
            {
                current_per_voice_midi_numbers = new_per_voice_midi_numbers;
                APVTS2ModelThread_midi_mappings_Que->push(new_per_voice_midi_numbers);
            }

            bExit = threadShouldExit();

            sleep (10);
        }
    }
    // ============================================================================================================

    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    bool readyToStop; // Used to check if thread is ready to be stopped or externally stopped from a parent thread
    // run this in destructor destructing object
    void prepareToStop(){
        //Need to wait enough to ensure the run() method is over before killing thread
        this->stopThread(20);
        readyToStop = true;
    }
    ~APVTSMediatorThread() override {
        if (not readyToStop)
        {
            prepareToStop();
        }
    }

    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================

    std::array<float, 4> get_groove_vel_offset_ranges()
    {
        return {
            *APVTS->getRawParameterValue("MIN_VELOCITY"), *APVTS->getRawParameterValue("MAX_VELOCITY"),
            *APVTS->getRawParameterValue("MIN_OFFSET"), *APVTS->getRawParameterValue("MAX_OFFSET"),
        };
    }
    
    std::array<float, HVO_params::num_voices> get_max_num_hits()
    {
        std::array<float, HVO_params::num_voices> max_num_hits {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            max_num_hits[i] = *APVTS->getRawParameterValue(voice_label+"_X");
        }
        return max_num_hits;
    }

    std::array<float, HVO_params::num_voices> get_sampling_thresholds()
    {
        std::array<float, HVO_params::num_voices> sampling_thresholds {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            sampling_thresholds[i] = *APVTS->getRawParameterValue(voice_label+"_Y");
        }
        return sampling_thresholds;
    }

    std::array<int, HVO_params::num_voices> get_per_voice_midi_numbers()
    {
        std::array<int, HVO_params::num_voices> midiNumbers {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            midiNumbers[i] = int(*APVTS->getRawParameterValue(voice_label + "_MIDI"));
        }
        return midiNumbers;
    }
private:
    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState* APVTS {nullptr};

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_Que {nullptr};
    LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_Que {nullptr};


};
