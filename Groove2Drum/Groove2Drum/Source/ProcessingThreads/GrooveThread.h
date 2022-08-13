//
// Created by Behzad Haki on 2022-08-05.
//

#ifndef JUCECMAKEREPO_GROOVETHREAD_H
#define JUCECMAKEREPO_GROOVETHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/Representations.h"
#include "../Includes/LockFreeQueueTemplate.h"
#include "../settings.h"

class GrooveThread:public juce::Thread
{
public:

    // constructor
    GrooveThread();

    void start_Thread(
        LockFreeQueue<Note, settings::processor_io_queue_size>* note_toProcess_quePntr,
        MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>* groove_toProcess_quePntr,
        LockFreeQueue<array<float, 4>, gui_io_queue_size>* veloff_fromGui_quePntr,
        MonotonicGrooveQueue<settings::time_steps, gui_io_queue_size>* groove_toGui_quePntr,
        StringLockFreeQueue<settings::gui_io_queue_size>* text_toGui_que_for_debuggingPntr = nullptr
        );

    // destructor
    ~GrooveThread() override;

    // run this in destructor destructing object
    void prepareToStop();

    // don't call this from the parent thread
    // instead, use startThread() from which
    // run() is explicitly called
    void run() override;


private:

    void NoteProcessor(Note latest_Note); // updates the internal groove
    void Send();                          // places the scaled groove in groove_toProcess_que


    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;


    // ---- Locally Used for calculations ----------------------------------
    LockFreeQueue<Note, settings::processor_io_queue_size>* note_toProcess_que{};    // queue for receiving the new notes
    MonotonicGrooveQueue<settings::time_steps,
                         processor_io_queue_size>* groove_toProcess_que{}; // the queue for sending the updated groove to the next thread
    //----------------------------------------------------------------------

    //---- Control Parameters from GUI -------------------------------------
    LockFreeQueue<array<float, 4>, gui_io_queue_size>* veloff_fromGui_que{};
    array<float, 2> vel_range;
    array<float, 2> offset_range;

    // ---- sends data to gui -----------------------------------------------
    // the queue for sending the updated groove to the next thread
    MonotonicGrooveQueue<settings::time_steps, gui_io_queue_size>* groove_toGui_que{};


    // ;
    //----------------------------------------------------------------------

    // ---- Locally Used for calculations ----------------------------------

    // an internal HVO instance of size {time_steps, 1 voice)
    // for tracking the unscaled groove
    MonotonicGroove<settings::time_steps> monotonic_groove;

    // torch::Tensor unscaled_groove_overdubbed;

    // an internal tensor of size { time_steps , 1}
    // for tracking the actual onset time of registered
    // notes in the groove_overdubbed tensor
    // torch::Tensor onset_ppqs_in_groove;

    // 32 grid line location (in ppq)
    torch::Tensor gridlines;

    // int find_nearest_gridline(float ppq);
    // bool shouldReplace(Note latest_Note);

    //----------------------------------------------------------------------

    //---- Debugger -------------------------------------
    StringLockFreeQueue<settings::gui_io_queue_size>* text_toGui_que_for_debugging{};
    //----------------------------------------------------------------------

};

#endif //JUCECMAKEREPO_GROOVETHREAD_H
