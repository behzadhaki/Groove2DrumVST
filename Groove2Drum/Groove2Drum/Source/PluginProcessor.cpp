#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){

    // GuiIOFifos
    GrooveThread2GGroovePianoRollWidgetQues = make_unique<GuiIOFifos::GrooveThread2GGroovePianoRollWidgetQues>();
    GroovePianoRollWidget2GrooveThreadQues = make_unique<GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues>();
    ModelThreadToDrumPianoRollWidgetQues = make_unique<GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues>();
    DrumPianoRollWidgetToModelThreadQues = make_unique<GuiIOFifos::DrumPianoRollWidgetToModelThreadQues>();
    // IntraProcessorFifos
    ProcessBlockToGrooveThreadQues = make_unique<IntraProcessorFifos::ProcessBlockToGrooveThreadQues>();
    GrooveThreadToModelThreadQues = make_unique<IntraProcessorFifos::GrooveThreadToModelThreadQues>();
    ModelThreadToProcessBlockQues = make_unique<IntraProcessorFifos::ModelThreadToProcessBlockQues>();

    // give access to resources and run threads
    modelThread.startThreadUsingProvidedResources(GrooveThreadToModelThreadQues.get(),
                                                  ModelThreadToProcessBlockQues.get(),
                                                  ModelThreadToDrumPianoRollWidgetQues.get(),
                                                  DrumPianoRollWidgetToModelThreadQues.get());

    grooveThread.startThreadUsingProvidedResources(ProcessBlockToGrooveThreadQues.get(),
                                                   GrooveThreadToModelThreadQues.get(),
                                                   GrooveThread2GGroovePianoRollWidgetQues.get(),
                                                   GroovePianoRollWidget2GrooveThreadQues.get());

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
    // get Playhead info and buffer size and sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();


    // STEP 2
    // check if new pattern is generated and available for playback
    if (ModelThreadToProcessBlockQues != nullptr)
    {
        if (ModelThreadToProcessBlockQues->new_generations.getNumReady() > 0)
        {
            latestGeneratedData = ModelThreadToProcessBlockQues->new_generations.getLatestOnly();
        }
    }

    // Step 3
    // In playback mode, add drum note to the buffer if the time is right
    if (Pinfo->getIsPlaying())
    {
        auto startPpq = *Pinfo->getPpqPosition();
        auto qpm = *Pinfo->getBpm();
        auto start_ = fmod(startPpq, HVO_params::time_steps/4); // start_ should be always between 0 and 8

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
        place_BasicNote_in_queue<GeneralSettings::processor_io_queue_size>(midiMessages, Pinfo, ProcessBlockToGrooveThreadQues.get(), fs);
        // place_BasicNote_in_queue<GeneralSettings::gui_io_queue_size>(midiMessages, Pinfo, ProcessBlockToGrooveThreadQue, fs);
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
