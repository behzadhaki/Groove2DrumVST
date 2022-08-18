#pragma once

#include "PluginProcessor.h"
#include "gui/MultiTabComponent.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    unique_ptr<MultiTabComponent> multiTabComponent;
};

