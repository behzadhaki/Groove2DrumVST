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
    // constexpr int playhead_queue_size { 64 };
    constexpr int note_queue_size { 512 };
    constexpr int torch_tensor_queue_size { 512 };

    // sample rate
    constexpr int sample_rate { 44100 };

    // model_settings
    constexpr char* default_model_path {"/Users/behzadhaki/Github/Groove2DrumVST/Groove2Drum/Groove2Drum/TorchScriptModels/misunderstood_bush_246-epoch_26_tst.pt"};
    constexpr int time_steps { 32 };
    constexpr int num_voices { 9 };
    }


#define DefaultModelPath  "/Users/behzadhaki/Github/Groove2DrumVST/Groove2Drum/Groove2Drum/TorchScriptModels/misunderstood_bush_246-epoch_26_tst.pt"

