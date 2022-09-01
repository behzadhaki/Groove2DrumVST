#pragma once

#include "PluginProcessor.h"
#include "gui/PianoRoll_GeneratedDrums.h"
#include "gui/PianoRoll_InteractiveMonotonicGroove.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer,
                              public juce::Button::Listener , public juce::Slider::Listener
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
    unique_ptr<GeneratedDrumsWidget> DrumsPianoRollWidget;
    unique_ptr<MonotonicGrooveWidget> MonotonicGroovePianoRollsWidget;

    // buttons for reseting groove or xyslider params
    juce::TextButton resetGrooveButton;
    juce::TextButton resetSamplingParametersButton;
    juce::TextButton resetAllButton;

    // playhead position progress bar
    double playhead_pos;
    juce::ProgressBar PlayheadProgressBar {playhead_pos};

    // vel offset ranges
    array<float, 4> VelOffRanges {HVO_params::_min_vel, HVO_params::_max_vel, HVO_params::_min_offset, HVO_params::_max_offset};

    void sliderValueChanged (juce::Slider* slider) override;

private:

    // sliders for groove manipulation
    juce::Slider minVelSlider;
    juce::Label  minVelLabel;
    unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minVelSliderAPVTSAttacher;
    juce::Slider maxVelSlider;
    juce::Label  maxVelLabel;
    unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxVelSliderAPVTSAttacher;
    juce::Slider minOffsetSlider;
    juce::Label  minOffsetLabel;
    juce::Slider maxOffsetSlider;
    juce::Label  maxOffsetLabel;



};

