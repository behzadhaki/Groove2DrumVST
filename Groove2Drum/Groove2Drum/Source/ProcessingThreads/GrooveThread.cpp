//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../settings.h"

using namespace torch::indexing;

GrooveEvent get_loc_and_offset (Note note_)
{
    // Converts a note to a groove event

    auto ppq = note_.time.ppq;
    std::vector<double> a{0, 0};
    auto _16_note_ppq = 0.25;
    auto _32_note_ppq = 0.125;
    auto _n_16_notes = 32;
    auto _max_offset = 0.5;

    auto div = round(ppq / _16_note_ppq);
    auto offset = (ppq - (div * _16_note_ppq)) /_32_note_ppq * _max_offset;
    auto index = fmod(div, _n_16_notes);

    return GrooveEvent(index, offset, note_.velocity);
}

GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{
    readyToStop = false;

    groove_overdubbed = torch::zeros({settings::time_steps, 3});
    onset_ppqs_in_groove = torch::zeros({settings::time_steps, 1});

    gridlines = torch::range(0, 7.9, 0.25);

}

void GrooveThread::start_Thread(
    LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQuePntr,
    LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>* scaledGrooveQuePntr,
    float* VelScaleParamPntr
)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    incomingNoteQue = incomingNoteQuePntr;
    VelScaleParam = VelScaleParamPntr;
    scaledGrooveQue = scaledGrooveQuePntr;

    startThread();

}

GrooveThread::~GrooveThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}

void GrooveThread::reset_groove()
{
    groove_overdubbed = torch::zeros({settings::time_steps, 3});
}

void GrooveThread::run()
{
    bool bExit = threadShouldExit();

    Note read_note;



    while (!bExit)
    {
        if (incomingNoteQue != nullptr)
        {
            while (incomingNoteQue->getNumReady() > 0 and not this->threadShouldExit())
            {
                incomingNoteQue->ReadFrom(&read_note, 1); // here cnt result is 3

                auto groove_event = get_loc_and_offset(read_note);

                DBG("Note PPQ " << read_note.time.ppq << " Pitch " << read_note.note << " Velocity " << read_note.velocity);
                DBG("Onset Index " << groove_event.grid_index << " Offset " << groove_event.offset << " Velocity " << groove_event.velocity  );
                /*NoteProcessor(read_note);
                GrooveScaler();
                Send();*/
            }
        }

        bExit = threadShouldExit();
        sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
    }
}

void GrooveThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}


void GrooveThread::NoteProcessor(Note latest_Note)
{
    // auto pitch = latest_Note.note;
    // auto ppq = latest_Note.time.ppq;
    // auto vel = latest_Note.velocity;
}

void GrooveThread::GrooveScaler()
{
    groove_overdubbed = groove_overdubbed * (*VelScaleParam);
}

void GrooveThread::Send()
{
    scaledGrooveQue->WriteTo(&groove_overdubbed, 1);
}




