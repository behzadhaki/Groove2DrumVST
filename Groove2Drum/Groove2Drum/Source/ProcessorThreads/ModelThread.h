//
// Created by Behzad Haki on 2022-08-13.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../InterThreadFifos.h"
#include "../settings.h"
#include "../Model/MonotonicV1ModelAPI.h"
#include "../Model/VAE_V1ModelAPI.h"

inline juce::StringArray get_monotonic_v1_pt_files_in_default_path()
{
    // find models in default folder
    juce::StringArray paths;
    paths.clear();
    for (const auto& filenameThatWasFound : juce::File (GeneralSettings::default_model_folder).findChildFiles (2, true, "*.pt"))
    {
        paths.add (filenameThatWasFound.getFullPathName());
    }
    paths.sort(false);
    return paths;
}

inline juce::StringArray get_vae_files_in_default_path()
{

    // find models in default folder
    juce::StringArray paths;
    paths.clear();
    for (const auto& filenameThatWasFound : juce::File (GeneralSettings::default_vae_model_folder).findChildFiles (2, true, "*.pt"))
    {
        paths.add (filenameThatWasFound.getFullPathName());
    }
    paths.sort(false);
    return paths;
}

inline juce::StringArray get_groove_converter_files_in_default_path()
{

    // find models in default folder
    juce::StringArray paths;
    paths.clear();
    paths.add("00_Instrument_Agnostic");
    for (const auto& filenameThatWasFound : juce::File (GeneralSettings::default_groove_converter_model_folder).findChildFiles (2, true, "*.pt"))
    {
        paths.add (filenameThatWasFound.getFullPathName());
    }
    paths.sort(false);
    return paths;
}

class ModelThread: public juce::Thread/*, public juce::ChangeBroadcaster*/
{
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    ModelThread();
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuesPntr,
        MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* ModelThread2GroovePianoRollWidgetQuePntr,
        GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>*  ModelThreadToProcessBlockQuesPntr,
        HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>* ModelThreadToDrumPianoRollWidgetQuesPntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices+2>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr,
        LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_QuePntr);
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override;
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    void prepareToStop();     // run this in destructor destructing object
    ~ModelThread() override;


    // ============================================================================================================
    // ===          Utility Methods && Parameters
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Per voice generation controls stored locally (defaults are in settings.h)
    // ------------------------------------------------------------------------------------------------------------
    vector<float> perVoiceSamplingThresholds {nine_voice_kit_default_sampling_thresholds};
    vector<float> perVoiceMaxNumVoicesAllowed {nine_voice_kit_default_max_voices_allowed};
    // ------------------------------------------------------------------------------------------------------------
    // ---         Input Groove && Generated HVO stored locally
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGroove<HVO_params::time_steps> scaled_groove;
    HVO<HVO_params::time_steps, HVO_params::num_voices > generated_hvo;
    // ------------------------------------------------------------------------------------------------------------
    // ---         Other
    // ------------------------------------------------------------------------------------------------------------
    bool readyToStop; // Used to check if thread is ready to be stopped || externally stopped from a parent thread

    void UpdateModelPath(std::string new_model_path_,
                         std::string new_instrument_specific_model_path_,
                         std::string sample_mode_);
    // ============================================================================================================


private:

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---          Output Queues
    // ------------------------------------------------------------------------------------------------------------
    GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>* ModelThreadToProcessBlockQue;
    HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>* ModelThreadToDrumPianoRollWidgetQue;
    // ------------------------------------------------------------------------------------------------------------
    // ---          Input Queues
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQue;
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* ModelThread2GroovePianoRollWidgetQue;
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_Que;
    LockFreeQueue<std::array<float, HVO_params::num_voices+2>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_Que;
    LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_Que;

    // ============================================================================================================
    // ===          Generative Torch Model
    // ============================================================================================================
    std::optional<MonotonicGrooveTransformerV1> monotonicV1modelAPI{std::nullopt};
    std::optional<VAE_V1ModelAPI> vaeV1ModelAPI{std::nullopt};
    std::optional<VAE_V1ModelAPI> I2G_GrooveConverterModelAPI{std::nullopt};
    array <int, HVO_params::num_voices> drum_kit_midi_map {};
    string new_model_path;
    string current_model_path;
    string new_instrument_specific_model_path;
    string current_instrument_specific_model_path;
    string sample_mode {"Threshold"}; //"Threshold" || "SampleProbability"
};

#endif //JUCECMAKEREPO_MODELTHREAD_H
