#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{

    // Create TextEditor for Text Messages
    multiTabComponent = make_unique<MultiTabComponent>(MidiFXProcessorPointer, 620, 600);
    multiTabComponent->setBounds (0, 0, 620, 700);
    addAndMakeVisible (multiTabComponent.get());

    // Set window size
    setSize (700, 800);
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


