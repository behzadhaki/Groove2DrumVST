//
// Created by Behzad Haki on 2022-08-05.
//

#ifndef JUCECMAKEREPO_GROOVETHREAD_H
#define JUCECMAKEREPO_GROOVETHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/Representations.h"
#include "../Includes/LockFreeQueueTemplate.h"

class GrooveThread:public juce::Thread
{
public:

    // constructor
    GrooveThread();

    void start_Thread(
        LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQuePntr,
        LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>* scaledGrooveQuePntr,
        float* VelScaleParamPntr,
        StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue_for_debuggingPntr = nullptr
        );

    // destructor
    ~GrooveThread() override;

    // run this in destructor destructing object
    void prepareToStop();

    // don't call this from the parent thread
    // instead, use startThread() from which
    // run() is explicitly called
    void run() override;

    // resets the overdubbed groove back to zero
    void reset_groove();

private:

    void NoteProcessor(Note latest_Note); // updates the internal groove
    void GrooveScaler();                  // scales the groove
    void Send();                          // places the scaled groove in scaledGrooveQue


    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;


    // ---- Locally Used for calculations ----------------------------------
    LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQue;    // queue for receiving the new notes
    LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>* scaledGrooveQue; // the queue for sending the updated groove to the next thread
    //----------------------------------------------------------------------

    //---- Control Parameters from GUI -------------------------------------
    float* VelScaleParam;
    //----------------------------------------------------------------------

    // ---- Locally Used for calculations ----------------------------------

    // an internal tensor of size {time_steps, 3)
    // for tracking the unscaled groove
    torch::Tensor groove_overdubbed;

    // an internal tensor of size { time_steps , 1}
    // for tracking the actual onset time of registered
    // notes in the groove_overdubbed tensor
    torch::Tensor onset_ppqs_in_groove;

    // 32 grid line location (in ppq)
    torch::Tensor gridlines;

    // int find_nearest_gridline(float ppq);
    // bool shouldReplace(Note latest_Note);

    //----------------------------------------------------------------------

    //---- Debugger -------------------------------------
    StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue_for_debugging;
    //----------------------------------------------------------------------

};

#endif //JUCECMAKEREPO_GROOVETHREAD_H
