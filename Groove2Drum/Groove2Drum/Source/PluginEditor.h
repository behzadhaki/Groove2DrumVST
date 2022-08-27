#pragma once

#include "PluginProcessor.h"
#include "gui/PianoRoll_GeneratedDrums.h"
#include "gui/PianoRoll_InteractiveMonotonicGroove.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer,
                              public juce::Button::Listener /*, public juce::ChangeListener*/
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    ~MidiFXProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    /*void changeListenerCallback (juce::ChangeBroadcaster* source) override;*/
    void timerCallback() override;
    void buttonClicked (juce::Button* button) override;

private:
    MidiFXProcessor* MidiFXProcessorPointer_;

    // gui widgets
    unique_ptr<PianoRoll_GeneratedDrums_AllVoices> DrumsPianoRollWidget;
    unique_ptr<MonotonicGrooveWidget> MonotonicGroovePianoRollsWidget;

    // buttons for reseting groove or xyslider params
    juce::TextButton resetGrooveButton;
    juce::TextButton resetSamplingParametersButton;
    juce::TextButton resetAllButton;

    // playhead position progress bar
    double playhead_pos;
    juce::ProgressBar PlayheadProgressBar {playhead_pos};
};

