#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    DBG("MidiFXProcessorEditor::MidiFXProcessorEditor()");
    MidiFXProcessorPointer_ = &MidiFXProcessorPointer;


    // initialize widgets
    GeneratedDrumsWidget = make_unique<FinalUIWidgets::GeneratedDrums::GeneratedDrumsWidget>(
        &MidiFXProcessorPointer_->apvts);
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_score =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            GeneratedDrumsWidget->updateWithNewScore(latest_score);
        }
    }

    DBG("MidiFXProcessorEditor::MidiFXProcessorEditor() 2");
    MonotonicGrooveWidget = make_unique<FinalUIWidgets::MonotonicGrooves::InputGrooveWidget>
        (MidiFXProcessorPointer_->get_pointer_GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue());
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_groove =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            MonotonicGrooveWidget->updateInputGroovesWithNewGroove(latest_groove);
        }

        auto ptr_2 = MidiFXProcessorPointer_->ModelThread2GroovePianoRollWidgetQue.get();
        if (ptr_2->getNumberOfWrites() > 0)
        {
            auto latest_groove =
                ptr_2->getLatestDataWithoutMovingFIFOHeads();
            MonotonicGrooveWidget->updateInputGroovesWithNewGroove(latest_groove);
        }
    }

    DBG("MidiFXProcessorEditor::MidiFXProcessorEditor() 3");

    // Add widgets to Main Editor GUI
    addAndMakeVisible(GeneratedDrumsWidget.get());
    addAndMakeVisible(MonotonicGrooveWidget.get());

    // Progress Bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    PlayheadProgressBar.setColour(PlayheadProgressBar.foregroundColourId, playback_progressbar_color);
    addAndMakeVisible(PlayheadProgressBar);

    // add buttons
    ButtonsWidget = make_unique<FinalUIWidgets::ButtonsWidget>(&MidiFXProcessorPointer_->apvts);
    addAndMakeVisible (ButtonsWidget.get());
    // ButtonsWidget->addListener(this);

    // initialize GrooveControlSliders
    ControlsWidget = make_unique<FinalUIWidgets::ControlsWidget> (&MidiFXProcessorPointer_->apvts);
    addAndMakeVisible (ControlsWidget.get());

    // model selector
    ModelSelectorWidget = make_unique<FinalUIWidgets::ModelSelectorWidget> (
        &MidiFXProcessorPointer_->apvts,
        "MODEL",
        MidiFXProcessorPointer_->model_paths,
        "I2G",
        MidiFXProcessorPointer_->instrument_model_paths,
        "SAMPLINGMETHOD");
    addAndMakeVisible (ModelSelectorWidget.get());


    // Set window size
    setResizable (true, true);
    setSize (1400, 1000);

    startTimer(50);

}

MidiFXProcessorEditor::~MidiFXProcessorEditor()
{
    //ButtonsWidget->removeListener(this);
    // ModelSelectorWidget->removeListener(this);
}

void MidiFXProcessorEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);                            // bounds for main Editor GUI

    // reserve right side for other controls
    area.removeFromRight(proportionOfWidth(gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));

    // layout pianoRolls for generated drums at top
    GeneratedDrumsWidget->setBounds (
        area.removeFromTop(
            area.proportionOfHeight(gui_settings::PianoRolls::completePianoRollHeight)));

    // layout pianoRolls for input groove
    MonotonicGrooveWidget->setBounds(
        area.removeFromBottom(
            area.proportionOfHeight(
                gui_settings::PianoRolls::completeMonotonicGrooveHeight)));


    // layout Playhead Progress Bar
    area.removeFromLeft(area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
    PlayheadProgressBar.setBounds(area.removeFromLeft(
        GeneratedDrumsWidget->PianoRolls[0]->getPianoRollSectionWidth()));

    // put buttons && GrooveControlSliders
    area = getLocalBounds();
    auto rightArea = area.removeFromRight(area.proportionOfWidth(gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));
    rightArea.removeFromLeft(rightArea.proportionOfWidth(0.1));
    rightArea.removeFromRight(rightArea.proportionOfWidth(0.1));
    ButtonsWidget->setBounds(rightArea.removeFromTop(rightArea.proportionOfHeight(0.3)));
    ModelSelectorWidget->setBounds(rightArea.removeFromTop(rightArea.proportionOfHeight(0.2)));
    ControlsWidget->setBounds(rightArea.removeFromBottom(rightArea.proportionOfHeight(0.5)));

}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));

}

void MidiFXProcessorEditor::timerCallback()
{
    // get Generations && probs from model thread to display on drum piano rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumReady() > 0)
        {
            GeneratedDrumsWidget->updateWithNewScore(
                ptr_->getLatestOnly());
        }
    }

    // get Grooves from GrooveThread to display in Groove Piano Rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();

        if (ptr_->getNumReady() > 0)
        {
            MonotonicGrooveWidget->updateInputGroovesWithNewGroove(ptr_->getLatestOnly());
        }

    }

    // get Mapped Grooves from GrooveThread to display in Groove Piano Rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->ModelThread2GroovePianoRollWidgetQue.get();

        if (ptr_->getNumReady() > 0)
        {
            MonotonicGrooveWidget->updateFinalDrumGrooveWithNewGroove(ptr_->getLatestOnly());
        }

    }

    // get playhead position to display on progress bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    GeneratedDrumsWidget->UpdatePlayheadLocation(playhead_pos);
}