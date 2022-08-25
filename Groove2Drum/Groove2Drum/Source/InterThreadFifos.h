//
// Created by Behzad Haki on 2022-08-21.
//

#pragma once
#include "settings.h"
#include "Includes/LockFreeQueueTemplate.h"
#include "Includes/CustomStructs.h"

// Lock-free Queues  for communicating data between main threads in the Processor
namespace IntraProcessorFifos
{
    // ========= processBlock() To GrooveThread =============================================
    // sends a received note from input to GrooveThread to update the input groove
    struct ProcessBlockToGrooveThreadQues
    {
        LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size> new_notes {};
    };
    // =================================================================================



    // ========= GrooveThread To Model Thread   =============================================
    struct GrooveThreadToModelThreadQues
    {
        MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>
            new_grooves {};
    };



    // ========= Model Thread  To ProcessBlock =============================================
    struct ModelThreadToProcessBlockQues
    {
        GeneratedDataQueue<HVO_params::time_steps,
                           HVO_params::num_voices,
                           GeneralSettings::processor_io_queue_size>
            new_generations {};
    };
}


/**
 *  Lock-free Queues  for communicating data back and forth between
 *  the Processor threads and the GUI widgets
 */
namespace GuiIOFifos
{

    // todo -> modelThread to gui ques for notifying if model loaded and also loading other models from a file browser

    // ========= Processor To TextEditor =============================================
    struct ProcessorToTextEditorQues
    {
        // used to communicate with BasicNote logger (i.e. display received notes in a textEditor)
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size> notes {};
        // Used for showing text messages on a text editor (only use in a single thread!!)
        StringLockFreeQueue<GeneralSettings::gui_io_queue_size> texts {}; // used for debugging only!
    };
    // =================================================================================



    // =========      PianoRoll_InteractiveMonotonicGroove FIFOs  ===================

    // used for sending velocities and offset compression values from MonotonicGroove XYSlider
    // to GrooveThread in processor
    struct GrooveThread2GGroovePianoRollWidgetQues
    {
        // used for sending the scaled groove to  MonotonicGroove Widget to display
        MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size> new_grooves {};       // todo To integrate in code
    };

    struct GroovePianoRollWidget2GrooveThreadQues
    {
        LockFreeQueue<array<float, 4>, GeneralSettings::gui_io_queue_size> newVelOffRanges {};                 // todo To integrate in code
        // used for sending a manually drawn note in the MonotonicGroove Widget to GrooveThread
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size> manually_drawn_notes {};          // todo To integrate in code
    };
    // =================================================================================



    // =========         Generated Drums PianoRoll            ========================
    // used for receiving the latest perVoiceSamplingThresholds and maximum notes allowed from Generated Drums XYSliders
    struct ModelThreadToDrumPianoRollWidgetQues
    {
        HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size> new_generated_data {};        // todo To integrate in code
    };

    struct DrumPianoRollWidgetToModelThreadQues
    {
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size> new_sampling_thresholds {};        // todo To integrate in code
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>  new_max_number_voices {};      // todo To integrate in code
    };
    // =================================================================================

}
