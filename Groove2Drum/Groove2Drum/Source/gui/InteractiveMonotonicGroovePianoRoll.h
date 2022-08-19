//
// Created by Behzad Haki on 2022-08-19.
//
#pragma once

#include "InteractivePianoRollBlock.h"

using namespace std;

class MonotonicGroovePianoRoll:public juce::Component
{
public:

    vector<shared_ptr<InteractivePianoRollBlock>> interactivePRollBlocks;
    juce::Colour def_c {juce::Colour::fromFloatRGBA(1.0f,1.0f,1.0f,1.0f)};
    juce::Colour beat_c { juce::Colour::fromFloatRGBA(.75f,.75f,.75f,1.0f)};
    juce::Colour bar_c {  juce::Colour::fromFloatRGBA(.4f,.4f,.4f,1.0f) };
    int num_gridlines;
    float step_ppq;
    juce::Label label;

    MonotonicGroovePianoRoll(bool isInteractive,int num_gridlines_, float step_ppq_, int n_steps_per_beat, int n_beats_per_bar, string label_text)
    {
        num_gridlines = num_gridlines_;
        step_ppq = step_ppq_;


        // Set Modified Label
        label.setText(label_text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible(label);

        // Draw up piano roll
        /*auto w_per_block = (int) size_width/num_gridlines;*/

        for (unsigned long i=0; i<time_steps; i++)
        {
            if (fmod(i, n_steps_per_beat*n_beats_per_bar) == 0)      // bar position
            {
                DBG("BAR");
                interactivePRollBlocks.push_back(make_shared<InteractivePianoRollBlock>(isInteractive, bar_c, i));
            }
            else if(fmod(i, n_steps_per_beat) == 0)                  // beat position
            {
                DBG("BEAT");
                interactivePRollBlocks.push_back(make_shared<InteractivePianoRollBlock>(isInteractive, beat_c, i));
            }
            else
            {
                DBG("OTHER");
                interactivePRollBlocks.push_back(make_shared<InteractivePianoRollBlock>(isInteractive, def_c, i));
            }

            addAndMakeVisible(interactivePRollBlocks[i].get());


        }

    }

    // location must be between 0 or 1
    void addEventToStep(int idx, float location, int hit_, float velocity_)
    {
        interactivePRollBlocks[idx]->addEvent(hit_, velocity_, location);
    }
    void addEventWithPPQ(float ppq_, int hit_, float velocity_)
    {
        auto idx = (unsigned long) (floor(ppq_/step_ppq));
        interactivePRollBlocks[idx]->addEventWithPPQ(hit_, velocity_, ppq_, step_ppq);
    }

    void resized() override {
        auto area = getLocalBounds();
        auto w = (float) getWidth();
        auto label_ratio_of_width = 0.1f;
        auto label_width = (int) (label_ratio_of_width * w);
        label.setBounds(area.removeFromLeft(label_width));
            
        auto grid_width = int((1.0f-label_ratio_of_width) * (0.95f*w) / (float) num_gridlines);
        for (int i = 0; i<num_gridlines; i++)
        {
            interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
        }
    }


};


class MonotonicGrooveWidget:public juce::Component
{
public:
    unique_ptr<MonotonicGroovePianoRoll> unModifiedGrooveGui;
    unique_ptr<MonotonicGroovePianoRoll> ModifiedGrooveGui;

    MonotonicGrooveWidget(int num_gridlines_, float step_ppq_duration, int n_steps_per_beat_, int n_beats_per_bar)
    {
        // Create Unmodified Piano ROll
        unModifiedGrooveGui = make_unique<MonotonicGroovePianoRoll>(true, num_gridlines_, step_ppq_duration, n_steps_per_beat_, n_beats_per_bar, "Unmodified Groove");
        addAndMakeVisible(unModifiedGrooveGui.get());
        // Create Unmodified Piano ROll
        ModifiedGrooveGui = make_unique<MonotonicGroovePianoRoll>(false, num_gridlines_, step_ppq_duration, n_steps_per_beat_, n_beats_per_bar, "Adjusted Groove");
        addAndMakeVisible(ModifiedGrooveGui.get());
    }

    void resized() override {
        auto area = getLocalBounds();
        auto height = int((float) area.getHeight() *0.45f);
        ModifiedGrooveGui->setBounds(area.removeFromTop(height));
        unModifiedGrooveGui->setBounds(area.removeFromBottom(height));

    }

};