#pragma once

#include "PluginProcessor.h"
#include "ProcessorToGuiQueueManagerThread.h"
#include "gui/PianoRoll_GeneratedDrums.h"
#include "gui/PianoRoll_InteractiveMonotonicGroove.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    ~MidiFXProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
private:
    MidiFXProcessor* MidiFXProcessorPointer_;

    // gui widgets
    unique_ptr<PianoRoll_GeneratedDrums_AllVoices> DrumsPianoRollWidget;
    unique_ptr<MonotonicGrooveWidget> MonotonicGroovePianoRollsWidget;

    // thread for managing incoming messages from processor
    ProcessorToGuiQueueManagerThread ProcessorToGuiQueueManagerThread_;

};

