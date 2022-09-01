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
    DrumsPianoRollWidget = make_unique<GeneratedDrumsWidget>(
        &MidiFXProcessorPointer_->apvts);
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_score =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            DrumsPianoRollWidget->updateWithNewScore(latest_score);
        }
    }

    MonotonicGroovePianoRollsWidget = make_unique<MonotonicGrooveWidget>
        (MidiFXProcessorPointer_->GroovePianoRollWidget2GrooveThreadQues.get());
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_groove =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            MonotonicGroovePianoRollsWidget->updateWithNewGroove(latest_groove);
        }
    }

    // Add widgets to Main Editor GUI
    addAndMakeVisible(DrumsPianoRollWidget.get());
    addAndMakeVisible(MonotonicGroovePianoRollsWidget.get());

    // Progress Bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    PlayheadProgressBar.setColour(PlayheadProgressBar.foregroundColourId, playback_progressbar_color);
    addAndMakeVisible(PlayheadProgressBar);

    // add buttons
    addAndMakeVisible (resetGrooveButton);
    resetGrooveButton.setButtonText ("Reset Groove");
    resetGrooveButton.addListener (this);
    addAndMakeVisible (resetSamplingParametersButton);
    resetSamplingParametersButton.setButtonText ("Reset Sampling Parameters");
    resetSamplingParametersButton.addListener (this);
    addAndMakeVisible (resetAllButton);
    resetAllButton.setButtonText ("Reset All");
    resetAllButton.addListener (this);

    // initialize GrooveControlSliders
    GrooveControlSliders = make_unique<UI::GrooveControlSliders> (&MidiFXProcessorPointer_->apvts);
    addAndMakeVisible (GrooveControlSliders.get());

    // Set window size
    setResizable (true, true);
    setSize (800, 400);

    startTimer(50);

}

MidiFXProcessorEditor::~MidiFXProcessorEditor()
{
    resetGrooveButton.removeListener(this);
    resetSamplingParametersButton.removeListener(this);
    resetAllButton.removeListener(this);
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
    PlayheadProgressBar.setBounds(area.removeFromLeft(DrumsPianoRollWidget->PianoRolls[0]->getPianoRollSectionWidth()));

    // GrooveControlSliders
    area = getLocalBounds();
    area.removeFromLeft(area.proportionOfWidth(1.0f - gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));
    area.removeFromTop(area.proportionOfHeight(0.9));
    GrooveControlSliders->setBounds(area);

    // put buttons
    area = getLocalBounds();
    area.removeFromLeft(area.proportionOfWidth(1.0f - gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));
    area.removeFromBottom(area.proportionOfHeight(0.1));
    area.removeFromTop(area.proportionOfHeight(0.9));
    auto gap_w = area.proportionOfWidth(.05f);
    auto button_w = area.proportionOfWidth(.2f);
    area.removeFromLeft(gap_w);
    resetGrooveButton.setBounds(area.removeFromLeft(button_w));
    area.removeFromLeft(gap_w);
    resetSamplingParametersButton.setBounds(area.removeFromLeft(button_w));
    area.removeFromLeft(gap_w);
    resetAllButton.setBounds(area.removeFromLeft(button_w));
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));

}

void MidiFXProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    /*if (slider == &minVelSlider)
    {
        VelOffRanges[0] = (float) minVelSlider.getValue();
        maxVelSlider.setValue(-VelOffRanges[0]);
        VelOffRanges[1] = -VelOffRanges[1];
    }
    else if (slider == &maxVelSlider)
    {
        VelOffRanges[1] = (float) maxVelSlider.getValue();
    }
    else if (slider == &minOffsetSlider)
    {
        VelOffRanges[2] = (float) minOffsetSlider.getValue();
        maxOffsetSlider.setValue(-VelOffRanges[2]);
        VelOffRanges[3] = -VelOffRanges[2];
    }
    else if (slider == &maxOffsetSlider)
    {
        VelOffRanges[3] = (float) maxOffsetSlider.getValue();
    }

    MidiFXProcessorPointer_->GroovePianoRollWidget2GrooveThreadQues->newVelOffRanges.push(VelOffRanges);

    DBG("NUM WRITES FOR RANGES " << MidiFXProcessorPointer_->GroovePianoRollWidget2GrooveThreadQues->newVelOffRanges.getLatestDataWithoutMovingFIFOHeads()[0]);
*/
}

void MidiFXProcessorEditor::timerCallback()
{
    // get Generations and probs from model thread to display on drum piano rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumReady() > 0)
        {
            DrumsPianoRollWidget->updateWithNewScore(
                ptr_->getLatestOnly());
        }
    }

    // get Grooves from GrooveThread to display in Groove Piano Rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();

        if (ptr_->getNumReady() > 0)
        {
            MonotonicGroovePianoRollsWidget->updateWithNewGroove(
                ptr_->getLatestOnly());
        }

    }

    // get playhead position to display on progress bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    DrumsPianoRollWidget->UpdatePlayheadLocation(playhead_pos);
}

void MidiFXProcessorEditor::buttonClicked (juce::Button* button)  // [2]
{

    if (button == &resetGrooveButton)
    {
        MidiFXProcessorPointer_->grooveThread.ForceResetGroove();
    }
    if (button == &resetSamplingParametersButton)
    {
        DBG("RESETING SamplingParams");
    }
    if (button == &resetAllButton)
    {
        DBG("RESETING ALL");
    }
}
