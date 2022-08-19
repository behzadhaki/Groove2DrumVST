#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{

    // Create TextEditor for Text Messages
    multiTabComponent = make_unique<MultiTabComponent>(MidiFXProcessorPointer, 900, 400, settings::time_steps, HVO_params::_16_note_ppq,settings::num_voices);
    multiTabComponent->setBounds (0, 0, 900, 400);
    addAndMakeVisible (multiTabComponent.get());

    //

    multiTabComponent->testComponent1->addEvent(1, .2f, 0.75);

    multiTabComponent->monotonicGroovePianoRoll->addEventWithPPQ(4.125f, 1, .9f);
    multiTabComponent->monotonicGroovePianoRoll->addEventToStep(1, .9f, 1, .2f);

    multiTabComponent->monotonicGrooveWidget->unModifiedGrooveGui->addEventWithPPQ(4.125f, 1, .9f);
    multiTabComponent->monotonicGrooveWidget->unModifiedGrooveGui->addEventWithPPQ(.253f, 1, .1f);

    // Set window size
    setSize (900, 400);
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


