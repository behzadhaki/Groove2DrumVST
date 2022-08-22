//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.


// ======================================================================================
// ==================     Drum Kit Defaults                ==============================
// ======================================================================================

// Voice Mappings corresponding to Kick, Snare, CH, OH, LoT, MidT, HiT, Crash, Ride
#define nine_voice_kit_default_sampling_thresholds std::vector<float>({0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5})
#define nine_voice_kit_default_midi_numbers std::vector<int>({36, 38, 42, 46, 43, 47, 50, 49, 51})
#define nine_voice_kit_labels std::vector<std::string>({"Kick", "Snare", "Closed Hat", "Open Hat", "Low Tom", "Mid Tom", "Hi Tom", "Crash", "Ride"})
#define monotonic_trigger std::vector<int> ({20, 21, 22})

// ======================================================================================
// ==================     General Settings                 ==============================
// ======================================================================================
namespace GeneralSettings
{
    // Queue Sizes for communication
    constexpr int gui_io_queue_size { 16 };
    constexpr int processor_io_queue_size { 64 };

    // model_settings
    // char constexpr* default_model_path {(char*)"/Users/behzadhaki/Github/Groove2DrumVST/Groove2Drum/Groove2Drum/Source/model/misunderstood_bush_246-epoch_26_tst.pt"};
    // FIXME add to readme.me for setup ==> model should be placed in root (/Library/Groove2Drum/trained_models) folder
    char constexpr* default_model_path {(char*)"/Library/Groove2Drum/trained_models/model.pt"};

}

// ======================================================================================
// ==================     HVO Settings                     ==============================
// ======================================================================================
namespace HVO_params
{
    constexpr int time_steps { 32 };
    constexpr int num_voices { 9 };
    constexpr double _16_note_ppq { 0.25 };         // each 16th note is 1/4th of a quarter
    constexpr double _32_note_ppq { 0.125 };        // each 32nd note is 1/8th of a quarter
    constexpr int _n_16_notes { 32 };               // duration of the HVO tensors is 32 16th notes (2 bars of 4-4)
    constexpr double _max_offset { 0.5 };           // offsets are defined in range (-0.5, 0.5)
    constexpr double _min_offset { -0.5 };          // offsets are defined in range (-0.5, 0.5)
    constexpr double _max_vel { 1 };                // velocities are defined in range (0, 1)
    constexpr double _min_vel { 1 };                // velocities are defined in range (0, 1)
}

// ======================================================================================
// ==================     Thread Settings                  ==============================
// ======================================================================================
namespace thread_settings{
//
    namespace GrooveThread
    {
        constexpr int waitTimeBtnIters {5}; //ms between two consecutive iterations of the thread loop in run()
    }
    namespace ModelThread
    {
        constexpr int waitTimeBtnIters {5}; //ms between two consecutive iterations of the thread loop in run()
    }
}

// ======================================================================================
// ==================        GUI Settings                  ==============================
// ======================================================================================
namespace gui_settings{
    //
    namespace BasicNoteStructLoggerTextEditor{
        constexpr int maxChars { 500 };
        constexpr int nNotesPerLine { 4 };
    }

    namespace TextMessageLoggerTextEditor{
        constexpr int maxChars { 3000 };
    }
}

