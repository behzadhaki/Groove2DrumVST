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
// ==========           safe to directly read from APVTS inside the processBlock() thread || any other
// ==========           time sensitive threads
// ==========
// ==========         In this plugin, the GrooveThread && the ModelThread are !time-sensitive. So,
// ==========           they can read from APVTS directly. Regardless, in future iterations, perhaps
// ==========           these requirements change. To be future-proof, this thread has been implemented
// ==========           to take care of mediating the communication of parameters in the APVTS to the
// ==========           ProcessorThreads as well as the processBlock()
// ============================================================================================================
class APVTSMediatorThread: public juce::Thread
{
public:
    juce::StringArray paths;
    juce::StringArray i2g_paths;

    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread(GrooveThread* grooveThreadPntr, ModelThread* modelThreadPntr): juce::Thread("APVTSMediatorThread")
    {
        grooveThread = grooveThreadPntr;
        modelThread = modelThreadPntr;
        auto monotonic_paths = get_monotonic_v1_pt_files_in_default_path();
        auto vae_folders = get_vae_files_in_default_path();
        // append the paths to the paths array
        paths.addArray(monotonic_paths);
        paths.addArray(vae_folders);
        // print the paths
        for (auto& path : paths)
        {
            std::cout << path << std::endl;
        }

        i2g_paths = get_groove_converter_files_in_default_path();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        juce::AudioProcessorValueTreeState* APVTSPntr,
        LockFreeQueue<std::array<float, 6>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr,
        LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices+2>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr,
        LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_QuePntr
        )
    {
        APVTS = APVTSPntr;
        APVTS2GrooveThread_groove_vel_offset_ranges_Que = APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr;
        APVTS2GrooveThread_groove_record_overdubToggles_Que = APVTS2GrooveThread_groove_record_overdubToggles_QuePntr;
        APVTS2ModelThread_max_num_hits_Que = APVTS2ModelThread_max_num_hits_QuePntr;
        APVTS2ModelThread_sampling_thresholds_and_temperature_Que = APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr;
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

        auto current_overdub_record_toggle_states = std::array<int, 2>{};
        auto current_groove_vel_offset_ranges = std::array<float, 6>{};
        auto current_max_num_hits = std::array<float, HVO_params::num_voices>{};
        auto current_sampling_thresholds_with_temperature_and_density =
            std::array<float, HVO_params::num_voices+2>{};
        auto current_per_voice_midi_numbers = std::array<int, HVO_params::num_voices> {};
        auto current_reset_buttons = std::array<int, 3> {};
        auto current_randomize_groove_buttons = std::array<int, 4>{};
        int current_model_selected = -1;
        int current_instrument_specific_model_selected = -1;
        string current_sampling_method;

        while (!bExit)
        {
            if (APVTS != nullptr)
            {
                auto new_overdub_record_toggle_states =
                    get_overdub_record_toggle_states();
                if (current_overdub_record_toggle_states
                    != new_overdub_record_toggle_states)
                {
                    current_overdub_record_toggle_states =
                        new_overdub_record_toggle_states;
                    APVTS2GrooveThread_groove_record_overdubToggles_Que->push(
                        new_overdub_record_toggle_states);
                }

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
                auto new_sampling_thresholds_with_temperature =
                    get_sampling_thresholds_with_temperature();

                if (current_sampling_thresholds_with_temperature_and_density
                    != new_sampling_thresholds_with_temperature)
                {
                    current_sampling_thresholds_with_temperature_and_density =
                        new_sampling_thresholds_with_temperature;
                    APVTS2ModelThread_sampling_thresholds_and_temperature_Que->push(
                        new_sampling_thresholds_with_temperature);
                }

                // check if per voice midi numbers have changed
                auto new_per_voice_midi_numbers = get_per_voice_midi_numbers();
                if (current_per_voice_midi_numbers != get_per_voice_midi_numbers())
                {
                    current_per_voice_midi_numbers = new_per_voice_midi_numbers;
                    APVTS2ModelThread_midi_mappings_Que->push(new_per_voice_midi_numbers);
                }

                // check if per reset buttons have been clicked
                auto new_reset_buttons = get_reset_buttons();
                if (current_reset_buttons != new_reset_buttons)
                {
                    auto resetGrooveButtonClicked = (current_reset_buttons[0] !=  new_reset_buttons[0]);
                    auto resetSampleParamsClicked = (current_reset_buttons[1] !=  new_reset_buttons[1]);
                    auto resetAllClicked = (current_reset_buttons[2] !=  new_reset_buttons[2]);
                    
                    if (resetGrooveButtonClicked || resetAllClicked)
                    {
                        grooveThread->ForceResetGroove();
                    }
                    if (resetSampleParamsClicked  || resetAllClicked)
                    {
                        // reset parameters to default
                        for(const string &ParamID : {"VEL_BIAS", "VEL_DYNAMIC_RANGE", "OFFSET_BIAS", "OFFSET_DYNAMIC_RANGE", "VEL_INVERT", "OFFSET_INVERT", "TEMPERATURE", "DENSITY"})
                        {
                            auto param = APVTS->getParameter(ParamID);
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
                        
                        for (size_t i=0; i < HVO_params::num_voices; i++)
                        {
                            auto ParamID = nine_voice_kit_labels[i];
                            auto param = APVTS->getParameter(ParamID+"_X");
                            param->setValueNotifyingHost(param->getDefaultValue());
                            param = APVTS->getParameter(ParamID+"_Y");
                            param->setValueNotifyingHost(param->getDefaultValue());
                            param = APVTS->getParameter(ParamID+"_MIDI");
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
                    }

                    // current_reset_buttons = new_reset_buttons;
                    reset_reset_buttons();
                }

                // check if random buttons have been clicked
                auto new_randomize_groove_buttons = get_randomize_groove_buttons();
                if (current_randomize_groove_buttons != new_randomize_groove_buttons)
                {
                    if (current_randomize_groove_buttons[0] != new_randomize_groove_buttons[0])
                    {
                        grooveThread->randomizeExistingVelocities();
                    }

                    if (current_randomize_groove_buttons[1] != new_randomize_groove_buttons[1])
                    {
                        grooveThread->randomizeExistingOffsets();
                    }

                    if (current_randomize_groove_buttons[2] != new_randomize_groove_buttons[2])
                    {
                        grooveThread->randomizeAll();
                    }

                    if (current_randomize_groove_buttons[3] != new_randomize_groove_buttons[3])
                    {
                        // check if vae is in model path
                        auto model_path = (string)paths[current_model_selected].toStdString();
                        auto vae_in_model_path = (model_path.find("vae") != string::npos);

                        // if not vae, then generate random groove
                        if (!vae_in_model_path)
                            grooveThread->randomizeAll();
                        else
                            modelThread->generateRandomPattern();
                    }


                    reset_random_buttons();
                }

                // check if new model selected
                auto new_model_selected = get_model_selected();
                auto new_instrument_specific_model_selected = get_instrument_specific_model_selected();
                auto new_sampling_method = get_sampling_method();

                if (current_model_selected != new_model_selected ||
                    current_sampling_method != new_sampling_method ||
                    current_instrument_specific_model_selected != new_instrument_specific_model_selected)
                {
                    current_model_selected = new_model_selected;
                    current_instrument_specific_model_selected = new_instrument_specific_model_selected;
                    current_sampling_method = new_sampling_method;
                    auto new_model_path = (string)paths[current_model_selected].toStdString();
                    auto new_instrument_specific_model_path =  (string)i2g_paths[current_instrument_specific_model_selected].toStdString();
                    std::cout << "new_instrument_specific_model_path xx" << new_instrument_specific_model_path << std::endl;
                    modelThread->UpdateModelPath(
                        new_model_path,
                        new_instrument_specific_model_path,
                        current_sampling_method);
                    rebroadcast();
                }

                bExit = threadShouldExit();

                // avoid burning CPU, if reading is returning immediately
                sleep (thread_settings::APVTSMediatorThread::waitTimeBtnIters);
            }
        }
    }
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    bool readyToStop {false}; // Used to check if thread is ready to be stopped || externally stopped from a parent thread

    // run this in destructor destructing object
    void prepareToStop(){
        //Need to wait enough to ensure the run() method is over before killing thread
        this->stopThread(100 * thread_settings::APVTSMediatorThread::waitTimeBtnIters);
        readyToStop = true;
    }

    ~APVTSMediatorThread() override {
        if (!readyToStop)
        {
            prepareToStop();
        }
    }

private:
    // ============================================================================================================
    // ===          Utility Methods && Parameters
    // ============================================================================================================

    std::array<int, 2> get_overdub_record_toggle_states()
    {
        return {
            int(*APVTS->getRawParameterValue("OVERDUB")), int(*APVTS->getRawParameterValue("RECORD"))
        };
    }

    std::array<float, 6> get_groove_vel_offset_ranges()
    {
        float vel_dynamic_range = *APVTS->getRawParameterValue("VEL_DYNAMIC_RANGE");
        int vel_invert = int(*APVTS->getRawParameterValue("VEL_INVERT"));
        float vel_bias = *APVTS->getRawParameterValue("VEL_BIAS") ;

        float offset_dynamic_range = *APVTS->getRawParameterValue("OFFSET_DYNAMIC_RANGE");
        int offset_invert = int(*APVTS->getRawParameterValue("OFFSET_INVERT"));
        float offset_bias = *APVTS->getRawParameterValue("OFFSET_BIAS");

        return {vel_bias, vel_dynamic_range, float(vel_invert), offset_bias, offset_dynamic_range, float(offset_invert)};
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

    std::array<float, HVO_params::num_voices+2> get_sampling_thresholds_with_temperature()
    {
        std::array<float, HVO_params::num_voices+2> sampling_thresholds_with_temperature {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            sampling_thresholds_with_temperature[i] = *APVTS->getRawParameterValue(voice_label+"_Y");
        }
        sampling_thresholds_with_temperature[HVO_params::num_voices] = *APVTS->getRawParameterValue("TEMPERATURE");
        sampling_thresholds_with_temperature[HVO_params::num_voices+1] = *APVTS->getRawParameterValue("DENSITY");
        return sampling_thresholds_with_temperature;
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

    // returns reset_groove, reset_sampling params && reset all
    std::array<int, 3> get_reset_buttons()
    {
        return {(int)*APVTS->getRawParameterValue("RESET_GROOVE"),
                (int)*APVTS->getRawParameterValue("RESET_SAMPLINGPARAMS"),
                (int)*APVTS->getRawParameterValue("RESET_ALL")};
    }

    // returns all reset buttons (buttons are toggles, so when mouse release they should jump back)
    void reset_reset_buttons()
    {
        auto param = APVTS->getParameter("RESET_GROOVE");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RESET_SAMPLINGPARAMS");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RESET_ALL");
        param->setValueNotifyingHost(0);
    }

    // returns RANDOMIZE_VEL, RANDOMIZE_OFFSET && RANDOMIZE_ALL all
    std::array<int, 4> get_randomize_groove_buttons()
    {
        return {(int)*APVTS->getRawParameterValue("RANDOMIZE_VEL"),
                (int)*APVTS->getRawParameterValue("RANDOMIZE_OFFSET"),
                (int)*APVTS->getRawParameterValue("RANDOMIZE_ALL"),
                (int)*APVTS->getRawParameterValue("RANDOM_GENERATION")};

    }

    // returns all reset buttons (buttons are toggles, so when mouse release they should jump back)
    void reset_random_buttons()
    {
        auto param = APVTS->getParameter("RANDOMIZE_VEL");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RANDOMIZE_OFFSET");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RANDOMIZE_ALL");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RANDOM_GENERATION");
        param->setValueNotifyingHost(0);
    }

    // returns RANDOMIZE_VEL, RANDOMIZE_OFFSET && RANDOMIZE_ALL all
    int get_model_selected()
    {
        auto model_selected = (int)*APVTS->getRawParameterValue("MODEL");
        return model_selected;
    }

    int get_instrument_specific_model_selected()
    {
        auto model_selected = (int)*APVTS->getRawParameterValue("I2G");
        return model_selected;
    }

    // returns RANDOMIZE_VEL, RANDOMIZE_OFFSET && RANDOMIZE_ALL all
    string get_sampling_method()
    {
        auto ix = (int)*APVTS->getRawParameterValue("SAMPLINGMETHOD");
        string model_selected = (ix == 0) ? "Threshold" : "SampleProbability";
        return model_selected;
    }

    // rebroadcasts the current state of the APVTS to all the queues
    void rebroadcast()
    {
        APVTS2GrooveThread_groove_record_overdubToggles_Que->push(get_overdub_record_toggle_states());
        APVTS2GrooveThread_groove_vel_offset_ranges_Que->push(get_groove_vel_offset_ranges());
        APVTS2ModelThread_max_num_hits_Que->push(get_max_num_hits());
        APVTS2ModelThread_sampling_thresholds_and_temperature_Que->push(get_sampling_thresholds_with_temperature());
        APVTS2ModelThread_midi_mappings_Que->push(get_per_voice_midi_numbers());
    }

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<std::array<float, 6>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_Que {nullptr};
    LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices+2>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_Que {nullptr};
    LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_Que {nullptr};

    // ============================================================================================================
    // ===          pointer to MidiFXProcessor
    // ============================================================================================================
    GrooveThread* grooveThread;
    ModelThread* modelThread;

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState* APVTS {nullptr};

};
