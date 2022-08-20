#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{

    //setResizeLimits (1278, 700, 1918, 1050);

    // const float ratio = 1.83f;
    // getConstrainer ()->setFixedAspectRatio (ratio);



    // Create TextEditor for Text Messages
    // todo move the definition to processor!! to keep state
    multiTabComponent = make_unique<MultiTabComponent>(MidiFXProcessorPointer/*, 800, 400*/, settings::time_steps, HVO_params::_16_note_ppq,settings::num_voices);
    addAndMakeVisible (multiTabComponent.get());

    // todo remove following tests

    multiTabComponent->monotonicGroovePianoRoll->addEventWithPPQ(4.125f, 1, .9f);
    multiTabComponent->monotonicGroovePianoRoll->addEventToStep(1, .9f, 1, .2f);

    multiTabComponent->monotonicGrooveWidget->unModifiedGrooveGui->addEventWithPPQ(4.125f, 1, .9f);
    multiTabComponent->monotonicGrooveWidget->unModifiedGrooveGui->addEventWithPPQ(.253f, 1, .1f);

    multiTabComponent->DrumsPianoRoll->addEventWithPPQ(0, 4.125f, 1, .9f, .25f);

    multiTabComponent->DrumsPianoRoll->addEventWithPPQ(7, 7.96f, 1, .2f, .9f);

    // Set window size

    setResizable (true, true);
    setSize (800, 400);


}

void MidiFXProcessorEditor::resized()
{

    auto area = getLocalBounds();
    setBounds(area);

    auto h = area.getHeight();

    multiTabComponent->setBounds (area.removeFromTop(int(h*.9f)));


}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


