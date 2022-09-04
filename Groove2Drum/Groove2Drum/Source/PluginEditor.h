#pragma once

#include "PluginProcessor.h"
#include "gui/CustomUIWidgets.h"

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
    unique_ptr<FinalUIWidgets::GeneratedDrums::GeneratedDrumsWidget> DrumsPianoRollWidget;
    unique_ptr<FinalUIWidgets::MonotonicGrooves::MonotonicGrooveWidget> MonotonicGroovePianoRollsWidget;

    // buttons for reseting groove or xyslider params
    juce::TextButton resetGrooveButton;
    juce::TextButton resetSamplingParametersButton;
    juce::TextButton resetAllButton;

    //  GrooveControlSliders
    unique_ptr<FinalUIWidgets::ControlsWidget> GrooveControlSliders;

    // playhead position progress bar
    double playhead_pos;
    juce::ProgressBar PlayheadProgressBar {playhead_pos};

    // vel offset ranges
    array<float, 4> VelOffRanges {HVO_params::_min_vel, HVO_params::_max_vel, HVO_params::_min_offset, HVO_params::_max_offset};

    void sliderValueChanged (juce::Slider* slider) override;





};

