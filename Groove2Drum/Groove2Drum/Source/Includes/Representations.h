//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../settings.h"
#include <torch/torch.h>

#include <utility>

using namespace std;
using namespace settings;

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
    onset_time() = default;

    // constructor for desired timing of event in terms of absolute ppq
    onset_time(double ppq_): ppq(ppq_) {}

    // constructor for received timing of midi messages
    // Automatically calculates absolute timing of event in terms of ppq
    onset_time(double frameStartPpq, double audioSamplePos, double qpm):ppq()
    {
        ppq = calculate_absolute_ppq(frameStartPpq, audioSamplePos, qpm); // calculate ppq
    }

    //constructor for calculating ppq using grid_line index and offset
    onset_time(int grid_line, double offset)
    {
        ppq = (offset / HVO_params::_max_offset * HVO_params::_32_note_ppq) + (grid_line * HVO_params::_16_note_ppq);
        if (ppq < 0)
        {
            ppq = ppq + HVO_params::_16_note_ppq * HVO_params::_n_16_notes;
        }
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
     * \n\n
     *
     * can create object three ways:
     *          \n\t\t 1. default empty note using Note()
     *          \n\t\t 2. notes received in AudioProcessorBlock using second constructor
     *                  (see place_note_in_queue() in Includes/UtilityMethods.h)
     *          \n\t\t 3. creating notes obtained from hvo (i.e. using ppq, note number and vel)
     *                  (see HVO::getNotes() below)
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

    bool capturedInLoop = false;        // useful for overdubbing calculations in
                                        // HVO::addNote(note_, shouldOverdub)
    bool capturedInPlaying = false;     // useful for checking if note was played while
                                        // playhead was playing
    // default constructor for empty instantiation
    Note() = default;

    // constructor for placing notes received in the processor from the MIDIBuffer
    Note(int note_number, float velocity_Value, double frameStartPpq, double audioSamplePos, double qpm):
        note(note_number), velocity(velocity_Value), time(frameStartPpq, audioSamplePos, qpm){
    }

    // constructor for placing notes generated
    Note(int voice_index, float velocity_Value, int grid_line, double offset,
         std::vector<int> voice_to_midi_map = nine_voice_kit):
        note(voice_to_midi_map[voice_index]), velocity(velocity_Value), time(grid_line, offset) {}

    // < operator to check which note happens earlier (used for sorting)
    bool operator<( const Note& rhs ) const
    { return time.ppq < rhs.time.ppq; }

    bool isLouderThan ( const Note& otherNote ) const
    {
        return velocity >= otherNote.velocity;
    }

    bool occursEarlierThan (const Note& otherNote ) const
    {
        return time.ppq < otherNote.time.ppq;
    }

    // converts the data to a string message to be printed/displayed
    string getStringDescription() const
    {
        std::ostringstream msg_stream;
        msg_stream << "N: " << note
                   << ", vel " << velocity
                   << ", time " << time.ppq
                   << " || " ;
        return msg_stream.str();
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
     *//*

struct GrooveEvent{

    int grid_index;
    double actual_onset_time;
    double offset;
    double velocity;

    GrooveEvent() = default;

    GrooveEvent(int grid_index_, double actual_onset_time_, double offset_, double velocity_):
        grid_index(grid_index_), actual_onset_time(actual_onset_time_), offset(offset_), velocity(velocity_)
    {}

    explicit GrooveEvent(Note note_)
    {
        // Converts a note to a groove event

        auto ppq = note_.time.ppq;
        auto div = round(ppq / HVO_params::_16_note_ppq);
        offset = (ppq - (div * HVO_params::_16_note_ppq)) / HVO_params::_32_note_ppq * HVO_params::_max_offset;
        grid_index = fmod(div, HVO_params::_n_16_notes);

        actual_onset_time = ppq;
        velocity = note_.velocity;

    }

    // converts the data to a string message to be printed/displayed
    string getStringDescription() const
    {
        std::ostringstream msg_stream;
        msg_stream << "groove_event " << actual_onset_time
                   << ", grid_index " << grid_index
                   << ", offset " << offset
                   << ", velocity " << velocity;
        return msg_stream.str();
    }

};
*/


/**
 * Converts a torch tensor to a string
 * @param torch::tensor
 * @return std::string
 */
inline std::string torch2string (const torch::Tensor& tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}


/// HVO structure for t time_steps and n num_voices
/// \n . Stores hits, velocities and offsets separately.
/// \n . can also create a random HVO quickly using HVO::Random()
/// \n . can automatically prepare and return notes extracted
///     from the HVO score see HVO::getNotes()
/// \tparam time_steps_
/// \tparam num_voices_
template <int time_steps_, int num_voices_> struct HVO
{
    int time_steps;
    int num_voices;
    torch::Tensor hits;
    torch::Tensor velocities_unmodified;
    torch::Tensor offsets_unmodified;
    torch::Tensor velocities_modified;
    torch::Tensor offsets_modified;
    


    /// Default Constructor
    HVO()
    {
        num_voices = num_voices_;
        time_steps = time_steps_;
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        velocities_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
    }

    /**
     *
     * @param hits_
     * @param velocities_
     * @param offsets_
     */
    HVO(torch::Tensor hits_, torch::Tensor velocities_, torch::Tensor offsets_):
        hits(std::move(hits_)), time_steps(time_steps_),
        num_voices(num_voices_),
        velocities_unmodified(std::move(velocities_)),
        offsets_unmodified(std::move(offsets_)), 
        velocities_modified(std::move(velocities_)),
        offsets_modified(std::move(offsets_))
    {
    }

    void reset()
    {
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        velocities_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
    }

    void randomize()
    {
        auto hits_old = torch::rand({time_steps, num_voices});
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::rand({time_steps, num_voices});
        offsets_unmodified = torch::rand({time_steps, num_voices}) - 0.5;

        // convert hits to 0 and 1s by thresholding
        auto row_indices = torch::arange(0, time_steps);
        for (int voice_i=0; voice_i < num_voices; voice_i++){
            // Get probabilities of voice hits at all timesteps
            auto voice_hot_probs = hits_old.index(
                {row_indices, voice_i});
            // Find locations exceeding threshold and set to 1 (hit)
            auto active_time_indices = voice_hot_probs>=0.5;
            hits.index_put_({active_time_indices, voice_i}, 1);
        }

        velocities_unmodified = velocities_unmodified * hits;
        offsets_unmodified = offsets_unmodified * hits;
        velocities_modified = velocities_unmodified * hits;
        offsets_modified = offsets_unmodified * hits;
    }

    vector<Note> getUnmodifiedNotes(std::vector<int> voice_to_midi_map = nine_voice_kit)
    {

        // get hit locations
        auto indices = hits.nonzero();
        auto n_notes = indices.sizes()[0];

        // empty vector for notes
        vector<Note> Notes;

        // for each index create note and add to vector
        for (int i=0; i<n_notes; i++)
        {
            int voice_ix =  indices[i][1].template item<int>();
            int grid_line = indices[i][0].template item<int>();
            auto velocity = velocities_unmodified[indices[i][0]][indices[i][1]].template item<float>();
            auto offset = offsets_unmodified[indices[i][0]][indices[i][1]].template item<double>();
            Note note_(voice_ix, velocity, grid_line, offset, voice_to_midi_map);
            Notes.push_back(note_);
        }

        // sort by time
        std::sort(Notes.begin(), Notes.end());

        return Notes;
    }

    vector<Note> getModifiedNotes(std::vector<int> voice_to_midi_map = nine_voice_kit)
    {

        // get hit locations
        auto indices = hits.nonzero();
        auto n_notes = indices.sizes()[0];

        // empty vector for notes
        vector<Note> Notes;

        // for each index create note and add to vector
        for (int i=0; i<n_notes; i++)
        {
            int voice_ix =  indices[i][1].template item<int>();
            int grid_line = indices[i][0].template item<int>();
            auto velocity = velocities_modified[indices[i][0]][indices[i][1]].template item<float>();
            auto offset = offsets_modified[indices[i][0]][indices[i][1]].template item<double>();
            Note note_(voice_ix, velocity, grid_line, offset, voice_to_midi_map);
            Notes.push_back(note_);
        }

        // sort by time
        std::sort(Notes.begin(), Notes.end());

        return Notes;
    }

    void compressVelocities(int voice_idx,
                            float min_val, float max_val)
    {
        auto hit_indices = hits > 0;

        for (int i=0; i<time_steps; i++)
        {
            velocities_modified[i][voice_idx]  =
                velocities_unmodified[i][voice_idx] * (max_val - min_val)
                + min_val;
        }

        velocities_modified = velocities_modified * hits;
    }

    void compressOffsets(int voice_idx,
                            float min_val, float max_val)
    {

        for (int i=0; i<time_steps; i++)
        {
            offsets_modified[i][voice_idx]  =
                (max_val - min_val)/(2*HVO_params::_max_offset)*
                    (offsets_unmodified[i][voice_idx]+HVO_params::_max_offset)
                +min_val;
        }

        offsets_modified = offsets_modified * hits;

    }

    virtual string getStringDescription(bool needScaled)
    {

        auto temp = getConcatenatedVersion(needScaled);

        std::ostringstream msg_stream;
        msg_stream << " HITS, VELOCITIES , OFFSETS " << endl << temp;

        return msg_stream.str();
    }

    torch::Tensor getConcatenatedVersion(bool needScaled)
    {
        if (needScaled)
        {
            return torch::cat({hits, velocities_modified, offsets_modified}, 1);
        }
        else
        {
            return torch::cat({hits, velocities_unmodified, offsets_unmodified}, 1);
        }
    }
};


/**
 * A structure holding the monotonic groove
 * @tparam time_steps_
 */
template <int time_steps_> struct MonotonicGroove
{
    torch::Tensor registeration_times;

    HVO<time_steps_, 1> hvo;    // holds the groove as is without modification

    MonotonicGroove()
    {
        registeration_times = torch::zeros({time_steps_, 1}, torch::kFloat32);

    }

    void resetGroove()
    {
        hvo.reset();
        registeration_times = torch::zeros({time_steps_, 1}, torch::kFloat32);
    }

    void ovrerdubWithNote(Note note_)
    {
        // only use notes that are received when host is playing
        if (note_.capturedInPlaying)
        {
            // 1. find the nearest grid line and calculate offset
            auto ppq = note_.time.ppq;
            auto div = round(ppq / HVO_params::_16_note_ppq);
            auto offset = (ppq - (div * HVO_params::_16_note_ppq))
                          / HVO_params::_32_note_ppq * HVO_params::_max_offset;
            auto grid_index = fmod(div, HVO_params::_n_16_notes);

            // 2. Place note in groove
            if (note_.velocity >=
                (hvo.velocities_unmodified[grid_index] * hvo.hits[grid_index]).template item<float>())
            {
                hvo.hits[grid_index] = 1;
                hvo.offsets_unmodified[grid_index] = offset;
                hvo.velocities_unmodified[grid_index] = note_.velocity;
                registeration_times[grid_index] = note_.time.ppq;
            }
        }

    }

    string getStringDescription(bool needScaled)
    {
        return hvo.getStringDescription(needScaled);
    }






};