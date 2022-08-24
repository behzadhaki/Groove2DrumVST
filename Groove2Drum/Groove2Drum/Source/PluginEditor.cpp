#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    MidiFXProcessorPointer_ = &MidiFXProcessorPointer;

    // get settings from HVO_params struct in Settings.h
    auto num_steps = HVO_params::time_steps;
    auto step_ppq_res = HVO_params::_16_note_ppq;
    auto steps_perBeat = HVO_params::num_steps_per_beat;
    auto beats_perBar = HVO_params::num_beats_per_bar;

    // initialize widgets
    DrumsPianoRollWidget = make_unique<PianoRoll_GeneratedDrums_AllVoices>(
        num_steps, step_ppq_res, steps_perBeat, beats_perBar,
        nine_voice_kit_labels, nine_voice_kit_default_midi_numbers);

    auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQues.get();
    if (ptr_->new_generated_data.getNumberOfWrites()>0)
    {
        auto latest_score = ptr_->new_generated_data.getLatestDataWithoutMovingFIFOHeads();
        DrumsPianoRollWidget->updateWithNewScore(latest_score);
    }


    MonotonicGroovePianoRollsWidget = make_unique<MonotonicGrooveWidget>
        (num_steps, step_ppq_res, steps_perBeat, beats_perBar);

    // Add widgets to Main Editor GUI
    addAndMakeVisible(DrumsPianoRollWidget.get());
    addAndMakeVisible(MonotonicGroovePianoRollsWidget.get());

    // start message manager thread
    // ProcessorToGuiQueueManagerThread_.startThreadUsingProvidedResources(DrumsPianoRollWidget.get(), MidiFXProcessorPointer.ModelThreadToDrumPianoRollWidgetQues.get());
    // todo for testing only
    // todo to remove later
    /*DrumsPianoRollWidget->addEventWithPPQ(7, 0.0f, 1, .2f, .9f);
    DrumsPianoRollWidget->addEventWithPPQ(4, 3.21f, 1, 0.5f, .4f);*/

    // Set window size
    setResizable (true, true);
    setSize (800, 400);

}

MidiFXProcessorEditor::~MidiFXProcessorEditor()
{
    MidiFXProcessorPointer_->modelThread.removeChangeListener(this);
    /*ProcessorToGuiQueueManagerThread_.signalThreadShouldExit();
    ProcessorToGuiQueueManagerThread_.stopThread(100);*/
}

void MidiFXProcessorEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);                            // bounds for main Editor GUI
    area.removeFromRight(proportionOfWidth(0.3f)); // reserve right side for other controls
    DrumsPianoRollWidget->setBounds (area.removeFromTop(area.proportionOfHeight(0.7f))); // piano rolls at top
    MonotonicGroovePianoRollsWidget->setBounds(area.removeFromBottom(area.proportionOfHeight(0.9f))); // groove at bottom
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}
void MidiFXProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &MidiFXProcessorPointer_->modelThread)
    {
        DBG("MODEL GENERATED NEW DATA!");
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQues.get();
        if (ptr_->new_generated_data.getNumReady()>0)
        {
            auto latest_score =
                ptr_->new_generated_data.getLatestOnly();
            DrumsPianoRollWidget->updateWithNewScore(latest_score);
        }
        //auto_latest_generated_data =
    }
    /*if (source == &MidiFXProcessorPointer_->grooveThread)
    {
        DBG("NEW GROOVE AVAILABLE");
    }*/
}
