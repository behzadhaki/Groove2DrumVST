//
// Created by Behzad Haki on 2022-08-21.
//

#pragma once
#include "settings.h"
#include "Includes/LockFreeQueueTemplate.h"
#include "Includes/CustomStructs.h"


/**
 *  Lock-free Queues  for communicating data between main threads in the Processor
 */
struct WithinMidiFXProcessorFifos
{
    // ========= processBlock() To GrooveThread =============================================
    // sends a received note from input to GrooveThread to update the input groove
    unique_ptr<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>>
        note_fromProcessBlockToGrooveThread_que;

    // ========= GrooveThread To Model Thread   =============================================
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>>
        groove_fromGrooveThreadToModelThread_que;

    // ========= Model Thread  To ProcessBlock =============================================
    unique_ptr<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>>
        GeneratedData_fromModelThreadToProcessBlock_que;


    // =============================================
    // ======  Constructor                   =======
    // =============================================
    WithinMidiFXProcessorFifos()
    {
        note_fromProcessBlockToGrooveThread_que = make_unique<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>> ();

        groove_fromGrooveThreadToModelThread_que = make_unique<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>>();

        GeneratedData_fromModelThreadToProcessBlock_que = make_unique<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>>();
    }

};


/**
 *  Lock-free Queues  for communicating data back and forth between
 *  the Processor threads and the GUI widgets
 */
struct GuiIOFifos
{
    // ========= Processor To TextEditor =============================================
    // used to communicate with BasicNote logger (i.e. display received notes in a textEditor)
    unique_ptr<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>> note_toGui_que;
    // Used for showing text messages on a text editor (only use in a single thread!!)
    unique_ptr<StringLockFreeQueue<GeneralSettings::gui_io_queue_size>>
        text_toGui_que_MainProcessBlockOnly; // used for debugging only!


    // =========      PianoRoll_InteractiveMonotonicGroove FIFOs  ===================
    // used for sending velocities and offset compression values from MonotonicGroove XYSlider
    // to GrooveThread in processor
    unique_ptr<LockFreeQueue<array<float, 4>, GeneralSettings::gui_io_queue_size>>
        velocityOffset_fromGui_que;                 // todo To integrate in code

    // used for sending the scaled groove to  MonotonicGroove Widget to display
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>> groove_toGui_que;       // todo To integrate in code

    // used for sending a manually drawn note in the MonotonicGroove Widget to GrooveThread
    unique_ptr<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>> note_drawn_manually_fromGui_que;          // todo To integrate in code


    // =========         Generated Drums PianoRoll            ========================
    // used for receiving the latest perVoiceSamplingThresholds and maximum notes allowed from Generated Drums XYSliders
    unique_ptr<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>  perVoiceSamplingThresh_fromGui_que;        // todo To integrate in code
    unique_ptr<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>  perVoiceMaxVoicesAllowed_fromGui_que;      // todo To integrate in code


    // =============================================
    // ======  Constructor                   =======
    // =============================================
    GuiIOFifos()
    {
        note_toGui_que = make_unique<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>>();
        text_toGui_que_MainProcessBlockOnly = make_unique<StringLockFreeQueue<GeneralSettings::gui_io_queue_size>>();

        velocityOffset_fromGui_que = make_unique<LockFreeQueue<array<float, 4>, GeneralSettings::gui_io_queue_size>>();
        groove_toGui_que = make_unique<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>>();
        note_drawn_manually_fromGui_que = make_unique<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>>();

        perVoiceSamplingThresh_fromGui_que = make_unique<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>();
        perVoiceMaxVoicesAllowed_fromGui_que = make_unique<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>();
    }

};
