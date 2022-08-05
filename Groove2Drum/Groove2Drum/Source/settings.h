//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

#include <torch/script.h> // One-stop header.

#define default_sampling_thresholds torch::tensor({0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5})

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

    // gui settings

}

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