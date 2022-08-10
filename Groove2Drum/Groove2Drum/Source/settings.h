//
// Created by Behzad Haki on 2022-02-11.
//

#ifndef JUCECMAKEREPO_SETTINGS_HPP
#define JUCECMAKEREPO_SETTINGS_HPP

#include <torch/script.h> // One-stop header.

#define default_sampling_thresholds torch::tensor({0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5})

// Voice Mappings corresponding to Kick, Snare, CH, OH, LoT, MidT, HiT, Crash, Ride
#define nine_voice_kit std::vector<int>({36, 38, 42, 46, 43, 47, 50, 49, 51})
#define monotonic_trigger std::vector<int> ({20, 21, 22})

// define your own namespace to hold constants
namespace settings
{
    // constants have internal linkage by default
    // constexpr int midi_queue_size { 64 };
    constexpr int note_queue_size { 512 };
    constexpr int torch_tensor_queue_size { 512 };
    constexpr int text_message_queue_size { 512 };

    // sample rate
    constexpr int sample_rate { 44100 };

    // model_settings
    char constexpr* default_model_path {(char*)"/Users/behzadhaki/Github/Groove2DrumVST/Groove2Drum/Groove2Drum/Source/model/misunderstood_bush_246-epoch_26_tst.pt"};
    constexpr int time_steps { 32 };
    constexpr int num_voices { 9 };


}
// hvo settings
namespace HVO_params
{
constexpr double _16_note_ppq { 0.25 };  // each 16th note is 1/4th of a quarter
constexpr double _32_note_ppq { 0.125 };  // each 32nd note is 1/8th of a quarter
constexpr int _n_16_notes { 32 };  // duration of the HVO tensors is 32 16th notes (2 bars of 4-4)
constexpr double _max_offset { 0.5 };  // offsets are defined in range (-0.5, 0.5)
}


// gui settings
namespace gui_settings{
    //
    namespace NoteStructLoggerTextEditor{
        constexpr int maxChars { 500 };
        constexpr int nNotesPerLine { 4 };
    }

    namespace TextMessageLoggerTextEditor{
        constexpr int maxChars { 3000 };
    }
}

namespace thread_settings{
//
namespace GrooveThread{
    constexpr int waitTimeBtnIters {1}; //ms between two consecutive iterations of the thread loop in run()
}

namespace TextMessageLoggerTextEditor{
    constexpr int maxChars { 3000 };
}
}

#endif //JUCECMAKEREPO_SETTINGS_HPP
