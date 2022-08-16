#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){




    //editor queues
    note_toGui_que = make_unique<LockFreeQueue<BasicNote, settings::gui_io_queue_size>>();
    text_toGui_que = make_unique<StringLockFreeQueue<settings::gui_io_queue_size>>();
    text_toGui_que_mainprocessBlockOnly = make_unique<StringLockFreeQueue<settings::gui_io_queue_size>>();

    // control paramer queues
    veloff_fromGui_que = make_unique<LockFreeQueue<array<float, 4>, gui_io_queue_size>>();
    perVoiceSamplingThresh_fromGui_que =
        make_unique<LockFreeQueue<std::array<float, settings::num_voices>,
            settings::gui_io_queue_size>>();


    //groove thread params
    note_toProcess_que =
        make_unique<LockFreeQueue<BasicNote, settings::processor_io_queue_size>>();
    groove_toProcess_que =
        make_unique<MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>>();

    GeneratedData_toProcessforPlayback_que =
        make_unique<LockFreeQueue<GeneratedData, settings::processor_io_queue_size>>();

    // queue for displaying the monotonicgroove in editor
    groove_toGui_que = make_unique<MonotonicGrooveQueue<settings::time_steps,
                                                       gui_io_queue_size>>();

    // give access to resources and run threads
    grooveThread.startThreadUsingProvidedResources(
        note_toProcess_que.get(),
        groove_toProcess_que.get(),
        veloff_fromGui_que.get(),
        groove_toGui_que.get(),
        text_toGui_que.get());

    modelThread.startThreadUsingProvidedResources(
        groove_toProcess_que.get(),
        perVoiceSamplingThresh_fromGui_que.get(),
        GeneratedData_toProcessforPlayback_que.get(),
        text_toGui_que.get());


    basicNoteStructLoggerTextEditor = make_shared<BasicNoteStructLoggerTextEditor>();
    textMessageLoggerTextEditor = make_shared<TextMessageLoggerTextEditor>();
    textMessageLoggerTextEditor_mainprocessBlockOnly = make_shared<TextMessageLoggerTextEditor>();

    // Create TextEditor for BasicNote Struct
    basicNoteStructLoggerTextEditor->setMultiLine (true);
    basicNoteStructLoggerTextEditor->setBounds (100, 40, 500, 100);
    basicNoteStructLoggerTextEditor->startThreadUsingProvidedResources(note_toGui_que.get());

    // Create TextEditor for Text Messages
    textMessageLoggerTextEditor->initialize("General");
    textMessageLoggerTextEditor->setMultiLine (true);
    textMessageLoggerTextEditor->setBounds (100, 200, 500, 100);
    textMessageLoggerTextEditor->startThreadUsingProvidedResources(text_toGui_que.get());

    // Create TextEditor for Text Messages
    textMessageLoggerTextEditor_mainprocessBlockOnly->initialize("ProcessBlockOnly");
    textMessageLoggerTextEditor_mainprocessBlockOnly->setMultiLine (true);
    textMessageLoggerTextEditor_mainprocessBlockOnly->setBounds (100, 400, 500, 100);
    textMessageLoggerTextEditor_mainprocessBlockOnly->startThreadUsingProvidedResources(text_toGui_que_mainprocessBlockOnly.get());


}

MidiFXProcessor::~MidiFXProcessor(){
    if (!modelThread.readyToStop)
    {
        modelThread.prepareToStop();
    }

    if (!grooveThread.readyToStop)
    {
        grooveThread.prepareToStop();
    }
}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    // STEP 1
    // get Playhead info and Add note and onset to note_toGui_que using BasicNote structure
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();

    maxFrameSizeinPpq = (settings::largest_buffer_size * (*Pinfo->getBpm())) / (60 * settings::sample_rate);

    if (GeneratedData_toProcessforPlayback_que != nullptr)
    {
        if (GeneratedData_toProcessforPlayback_que->getNumReady() > 0)
        {
            latestGeneratedData = GeneratedData_toProcessforPlayback_que->getLatestOnly();
            DBG("GENERATION REceived2");
            if (latestGeneratedData.onset_ppqs.size()> 0)
            {
                showMessageinEditor(text_toGui_que_mainprocessBlockOnly.get(), std::to_string((int) latestGeneratedData.onset_ppqs.size()), "number of Generated notes", false);
            }
        }
    }

    if (not midiMessages.isEmpty() /*and groove_thread_ready*/)
    {
        // send BasicNotes to the GrooveThread and also gui logger for notes
        place_BasicNote_in_queue<settings::gui_io_queue_size>(midiMessages, Pinfo, note_toGui_que.get());
        place_BasicNote_in_queue<settings::processor_io_queue_size>(midiMessages, Pinfo, note_toProcess_que.get());
    }

    midiMessages.swapWith(tempBuffer);


    buffer.clear(); //

}

juce::AudioProcessorEditor* MidiFXProcessor::createEditor()
{
    return new MidiFXProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiFXProcessor();
}
