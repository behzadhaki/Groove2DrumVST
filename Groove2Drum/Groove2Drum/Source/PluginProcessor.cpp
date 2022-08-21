#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){


    processorGuiFIFOS = make_unique<GuiIOFIFOS>();

    withinMidiFXProcessorFIFOs = make_unique<WithinMidiFXProcessorFIFOs>();

    // give access to resources and run threads
    modelThread.startThreadUsingProvidedResources(
        withinMidiFXProcessorFIFOs->groove_fromGrooveThreadtoModelThread_que.get(),
        processorGuiFIFOS->perVoiceSamplingThresh_fromGui_que.get(),
        withinMidiFXProcessorFIFOs->GeneratedData_fromModelThreadtoProcessBlock_que.get()/*,
        processorGuiFIFOS->text_toGui_que.get()*/);

    grooveThread.startThreadUsingProvidedResources(
        withinMidiFXProcessorFIFOs->note_fromProcessBlockToGrooveThread_que.get(),
        withinMidiFXProcessorFIFOs->groove_fromGrooveThreadtoModelThread_que.get(),
        processorGuiFIFOS->veloff_fromGui_que.get(),
        processorGuiFIFOS->groove_toGui_que.get()/*,
        processorGuiFIFOS->text_toGui_que.get()*/);

   /* basicNoteStructLoggerTextEditor = make_shared<BasicNoteStructLoggerTextEditor>(note_toGui_que.get());
    textMessageLoggerTextEditor = make_shared<TextMessageLoggerTextEditor>( "General", text_toGui_que.get());
    textMessageLoggerTextEditor_mainprocessBlockOnly = make_shared<TextMessageLoggerTextEditor>("ProcessBlockOnly", text_toGui_que_mainprocessBlockOnly.get());
*/
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

    if (withinMidiFXProcessorFIFOs->GeneratedData_fromModelThreadtoProcessBlock_que != nullptr)
    {
        if (withinMidiFXProcessorFIFOs->GeneratedData_fromModelThreadtoProcessBlock_que->getNumReady() > 0)
        {
            latestGeneratedData = withinMidiFXProcessorFIFOs->GeneratedData_fromModelThreadtoProcessBlock_que->getLatestOnly();
        }
    }


if (Pinfo->getIsPlaying())

    {
        auto startPpq = *Pinfo->getPpqPosition();
        auto qpm = *Pinfo->getBpm();
        auto start_ = fmod(startPpq, settings::time_steps/4); // start_ should be always between 0 and 8
        auto fs = getSampleRate();
        auto buffSize = buffer.getNumSamples();

        //juce::MidiMessage msg = juce::MidiMessage::noteOn((int)1, (int)36, (float)100.0);
        if (latestGeneratedData.numberOfGenerations() > 0)
        {
            for (int idx = 0; idx < latestGeneratedData.numberOfGenerations(); idx++)
            {
                auto ppqs_from_start_ = latestGeneratedData.ppqs[idx] - start_;
                auto samples_from_start_ = ppqs_from_start_ * (60 * fs) / qpm;

                if (ppqs_from_start_>=0 and samples_from_start_<buffSize)
                {
                    // send note on
                    tempBuffer.addEvent(latestGeneratedData.midiMessages[idx], (int) samples_from_start_);
                    // send note off
                    tempBuffer.addEvent(juce::MidiMessage::noteOff((int) 1, (int) latestGeneratedData.midiMessages[idx].getNoteNumber(), (float) 0), (int) samples_from_start_);
                }

            }
        }
    }


    if (not midiMessages.isEmpty() /*and groove_thread_ready*/)
    {
        // send BasicNotes to the GrooveThread and also gui logger for notes
        place_BasicNote_in_queue<settings::gui_io_queue_size>(midiMessages, Pinfo, processorGuiFIFOS->note_toGui_que.get());
        place_BasicNote_in_queue<settings::processor_io_queue_size>(midiMessages, Pinfo, withinMidiFXProcessorFIFOs->note_fromProcessBlockToGrooveThread_que.get());
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
