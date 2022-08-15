//
// Created by Behzad Haki on 2022-02-11.
//

#include "../settings.h"
#include <torch/torch.h>
#include "Representations.h"

// can be used in processor to pass the messages received in a MidiBuffer as is,
// sequentially in a queue to be shared with other threads

using namespace std;

/**
 * converts a note_on message received in the processor into a BasicNote instance
 * then sends it to receiver thread using the provided note_que
 * @param midiMessages (juce::MidiBuffer&)
 * @param playheadP (juce::AudioPlayHead*)
 * @param note_que  (LockFreeQueue<Note, que_size>*)
 *
 */

template<int que_size>
inline void place_BasicNote_in_queue(
    juce::MidiBuffer& midiMessages,
    juce::AudioPlayHead* playheadP,
    LockFreeQueue<BasicNote, que_size>* note_que)
{
    double frameStartPpq = 0;
    double qpm = 0;
    bool isPlaying = false;
    bool isLooping = false;

    if (playheadP)
    {
        juce::AudioPlayHead::CurrentPositionInfo position;

        if (playheadP->getCurrentPosition (position))
        {

            // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
            // read from midi_message_que
            frameStartPpq = position.ppqPosition;
            qpm = position.bpm;
            isPlaying = position.isPlaying;
            isLooping = position.isLooping;

            for (auto m: midiMessages)
            {
                auto message = m.getMessage();
                if (message.isNoteOn())
                {
                    BasicNote note(message.getNoteNumber(),
                              message.getFloatVelocity(),
                              frameStartPpq,
                              message.getTimeStamp(),
                              qpm);
                    note.capturedInPlaying = isPlaying;
                    note.capturedInLoop = isLooping;
                    note_que->WriteTo(&note, 1);
                }
            }

        }
    }
}


inline string stream2string(std::ostringstream msg_stream)
{
    return msg_stream.str();
}

/**
 * Sends a string along with header to a logger thread using the specified queue
 *
 * @param text_message_queue (StringLockFreeQueue<settings::text_message_queue_size>*):
 *                          queue for communicating with message receiver thread
 * @param message (string): main message to display
 * @param header  (string): message to be printed as a header before showing message
 * @param clearFirst (bool): if true, empties receiving gui thread before display
 */
inline void showMessageinEditor(StringLockFreeQueue<settings::gui_io_queue_size>* text_message_queue,
                                string message, string header, bool clearFirst)
{
    if (text_message_queue!=nullptr)
    {
        if (clearFirst)
        {
            text_message_queue->addText((char*) "clear");
        }

        text_message_queue->addText("<<<<<  " + header + "  >>>>>>");

        /*char* c_message = const_cast<char*>(message.c_str());
        text_message_queue->addText(c_message);*/

        text_message_queue->addText(message);
    }

}


// converts a tensor to string to be used with DBG for debugging
inline std::string tensor2string (torch::Tensor tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}