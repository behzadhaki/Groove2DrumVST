//
// Created by Behzad Haki on 2022-02-11.
//

#ifndef CONSTANTS_SETTINGS_H
#define CONSTANTS_SETTINGS_H

// define your own namespace to hold constants
namespace settings
{
    // constants have internal linkage by default
    constexpr int midi_queue_size { 512 };
    constexpr int playhead_queue_size { 512 };
    constexpr int note_queue_size { 512 };
    constexpr int torch_tensor_queue_size {16};

    // sample rate
    constexpr int sample_rate { 44100 };
}

#endif //CONSTANTS_SETTINGS_H
