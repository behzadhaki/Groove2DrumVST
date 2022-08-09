//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../settings.h"

using namespace std;

/**the onset of midi message has two attributes: (1) the ppq of the beginning of the frame
     * (2) the number of AUDIO samples from the beginning of the frame
     * hence we need to use the sample rate and the qpm from the daw to calculate
     * absolute ppq of the onset. Look into NppqPosB structure
     *
     * \param ppq (double): time in ppq
     *
     * Used in Note Structure (see for instantiation example)
     */
struct onset_time{

    // exact position of onset with respect to ppq
    double ppq;

    // default constructor for empty instantiation
    onset_time(){}

    // constructor for desired timing of event in terms of absolute ppq
    onset_time(double ppq_): ppq(ppq_) {}

    // constructor for received timing of midi messages
    // Automatically calculates absolute timing of event in terms of ppq
    onset_time(double frameStartPpq, double audioSamplePos, double qpm):ppq()
    {
        ppq = calculate_absolute_ppq(frameStartPpq, audioSamplePos, qpm); // calculate ppq
    }

    // used usually for finding absolute timing of messages received in processor MIDIBuffer
    double calculate_absolute_ppq(double frameStartPpq, double audioSamplePos, double qpm)
    {
        auto tmp_ppq = frameStartPpq + audioSamplePos * qpm / (60 * settings::sample_rate);
        return tmp_ppq;
    }

    // used usually for preparing for playback through the processor MIDI buffer
    juce::int64 calculate_num_samples_from_frame_ppq(double frameStartPpq, double qpm){
        auto tmp_audioSamplePos = (ppq - frameStartPpq) * 60.0 * settings::sample_rate / qpm;
        return juce::int64(tmp_audioSamplePos);
    }

};


/**
     * A note structure holding the note number for an onset along with
     * ppq position -->  defined as the ration of quarter note
     * onset_time (struct see above) --> structure holding absolute ppq with implemented
     * utilities for quick conversions for buffer to processsor OR processor to buffer  cases
     *
     * \param note (int): midi note number of the Note object
     * \param velocity (float): velocity of the note
     * \param time (onset_time): structure for onset time of the note (see onset_time)
     *
     */
struct Note{

    int note;
    float velocity;
    onset_time time;
    // default constructor for empty instantiation
    Note(){}

    // constructor for placing notes received in the processor from the MIDIBuffer
    Note(int note_number, float velocity_Value, double frameStartPpq, double audioSamplePos, double qpm):
        note(note_number), velocity(velocity_Value), time(frameStartPpq, audioSamplePos, qpm){
    }
};


/**
     * Structure holding the information regarding a registered note to be placed in
     * groove_buffer
     *
     * \param grid_index (int): grid index closest to the onset
     * \param actual_onset_time (double): actual ppq of the registered onset
     * \param  offset (double): deviation from the gridline
     * \param velocity (double): velocity of note
     */

struct GrooveEvent{

    int grid_index;
    double actual_onset_time;
    double offset;
    double velocity;

    GrooveEvent()
    {}

    GrooveEvent(int grid_index_, double actual_onset_time_, double offset_, double velocity_):
        grid_index(grid_index_), actual_onset_time(actual_onset_time_), offset(offset_), velocity(velocity_)
    {}

    GrooveEvent(Note note_)
    {
        // Converts a note to a groove event

        auto ppq = note_.time.ppq;
        auto _16_note_ppq = 0.25;
        auto _32_note_ppq = 0.125;
        auto _n_16_notes = 32;
        auto _max_offset = 0.5;
        auto div = round(ppq / _16_note_ppq);

        offset = (ppq - (div * _16_note_ppq)) /_32_note_ppq * _max_offset;
        grid_index = fmod(div, _n_16_notes);

        actual_onset_time = ppq;
        velocity = note_.velocity;

    }

    // converts the data to a string message to be printed/displayed
    string getStringDescription()
    {
        std::ostringstream msg_stream;
        msg_stream << "groove_event " << actual_onset_time
                   << ", grid_index " << grid_index
                   << ", offset " << offset
                   << ", velocity " << velocity;
        return msg_stream.str();
    }

};
