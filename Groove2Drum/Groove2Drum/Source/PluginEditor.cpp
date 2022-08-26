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
        nine_voice_kit_labels, nine_voice_kit_default_midi_numbers,
        MidiFXProcessorPointer.DrumPianoRollWidgetToModelThreadQues.get(),
        MidiFXProcessorPointer.modelThread.perVoiceSamplingThresholds, MidiFXProcessorPointer.modelThread.perVoiceMaxNumVoicesAllowed);

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
    DrumsPianoRollWidget->addEventWithPPQ(4, 3.21f, 1, 0.1f, .4f);
    DrumsPianoRollWidget->addEventWithPPQ(4, 3.01f, 0, 0.5f, 0.0f);
    DrumsPianoRollWidget->addEventWithPPQ(4, 4.01f, 1, 0.5f, 0.0f);*/

    // Progress Bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    addAndMakeVisible(PlayheadProgressBar);

    // Set window size
    setResizable (true, true);
    setSize (800, 400);

    startTimer(50);

}

MidiFXProcessorEditor::~MidiFXProcessorEditor()
{
    /*MidiFXProcessorPointer_->modelThread.removeChangeListener(this);*/
    /*ProcessorToGuiQueueManagerThread_.signalThreadShouldExit();
    ProcessorToGuiQueueManagerThread_.stopThread(100);*/
}

void MidiFXProcessorEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);                            // bounds for main Editor GUI

    // reserve right side for other controls
    area.removeFromRight(proportionOfWidth(gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));

    // layout pianoRolls for generated drums at top
    DrumsPianoRollWidget->setBounds (area.removeFromTop(area.proportionOfHeight(gui_settings::PianoRolls::completePianoRollHeight))); // piano rolls at top

    // layout pianoRolls for generated drums on top
    MonotonicGroovePianoRollsWidget->setBounds(area.removeFromBottom(area.proportionOfHeight(gui_settings::PianoRolls::completeMonotonicGrooveHeight))); // groove at bottom

    // layout Playhead Progress Bar
    area.removeFromLeft(area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
    area.removeFromRight(area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width*1.2f));
    PlayheadProgressBar.setBounds(area);

}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}
void MidiFXProcessorEditor::timerCallback()
{
    // get Generations and probs from model thread to display on drum piano rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQues.get();
        if (ptr_->new_generated_data.getNumReady() > 0)
        {
            DrumsPianoRollWidget->updateWithNewScore(
                ptr_->new_generated_data.getLatestOnly());
        }
    }

    // get Grooves from GrooveThread to display in Groove Piano Rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQues.get();

        if (ptr_->new_grooves.getNumReady() > 0)
        {
            DBG("RECEIVED NEW GROOVE");
            MonotonicGroovePianoRollsWidget->updateWithNewGroove(
                ptr_->new_grooves.getLatestOnly());
        }

    }

    // get playhead position to display on progress bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();

}
