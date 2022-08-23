#pragma once

#include "PluginProcessor.h"
#include "ProcessorToGuiQueueManagerThread.h"
#include "gui/PianoRoll_GeneratedDrums.h"
#include "gui/PianoRoll_InteractiveMonotonicGroove.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    ~MidiFXProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // gui widgets
    unique_ptr<PianoRoll_GeneratedDrums_AllVoices> DrumsPianoRollWidget;
    unique_ptr<MonotonicGrooveWidget> MonotonicGroovePianoRollsWidget;

    // thread for managing incoming messages from processor
    ProcessorToGuiQueueManagerThread ProcessorToGuiQueueManagerThread_;
};

