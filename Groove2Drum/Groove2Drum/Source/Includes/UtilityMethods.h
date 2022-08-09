//
// Created by Behzad Haki on 2022-02-11.
//

#include "../settings.h"

// can be used in processor to pass the messages received in a MidiBuffer as is,
// sequentially in a queue to be shared with other threads

using namespace std;

inline void place_note_in_queue(
    juce::MidiBuffer& midiMessages,
    juce::AudioPlayHead* playheadP,
    LockFreeQueue<Note, settings::note_queue_size>* note_que)
{
    double frameStartPpq = 0;
    double qpm = 0;

    if (playheadP)
    {
        juce::AudioPlayHead::CurrentPositionInfo position;

        if (playheadP->getCurrentPosition (position))
        {
            // if (position.isPlaying)
            if (true) // todo replace with previous
            {
                // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
                // read from midi_message_que
                frameStartPpq = position.ppqPosition;
                qpm = position.bpm;

                for (auto m: midiMessages)
                {
                    auto message = m.getMessage();
                    if (message.isNoteOn())
                    {
                        Note note(message.getNoteNumber(),
                                  message.getFloatVelocity(),
                                  frameStartPpq,
                                  message.getTimeStamp(),
                                  qpm);
                        note_que->WriteTo(&note, 1);
                    }
                }
            }
        }
    }
}


inline string stream2string(std::ostringstream msg_stream)
{
    return msg_stream.str();
}



inline void showMessageinEditor(StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue,
                                string message, string header, bool clearFirst)
{
    if (text_message_queue!=nullptr)
    {
        if (clearFirst)
        {
            text_message_queue->addText((char*) "clear");
        }

        text_message_queue->addText(header);

        /*char* c_message = const_cast<char*>(message.c_str());
        text_message_queue->addText(c_message);*/

        text_message_queue->addText(message);
        text_message_queue->addText((char*) "---------------------");
    }


}