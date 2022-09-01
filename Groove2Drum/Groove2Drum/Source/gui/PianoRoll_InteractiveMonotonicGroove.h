//
// Created by Behzad Haki on 2022-08-19.
//
#pragma once

#include "CustomUIWidgets.h"

using namespace std;

/***
 * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
 * row to represent either the piano roll for the unmodified (interactive) OR the modified (non-interactive) sections of
 * of the piano roll for the Input Groove.
 */
class PianoRoll_InteractiveMonotonicGroove :public juce::Component
{
public:

    vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlock>> interactivePRollBlocks;
    juce::Colour def_c {juce::Colour::fromFloatRGBA(1.0f,1.0f,1.0f,0.8f)};
    juce::Colour beat_c { juce::Colour::fromFloatRGBA(.75f,.75f,.75f, 0.5f)};
    juce::Colour bar_c {  juce::Colour::fromFloatRGBA(.6f,.6f,.6f, 0.5f) };
    juce::Label label;


    PianoRoll_InteractiveMonotonicGroove(bool isInteractive, string label_text,
                                         GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQues = nullptr)
    {

        // Set Modified Label
        label.setText(label_text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible(label);

        // Draw up piano roll
        /*auto w_per_block = (int) size_width/num_gridlines;*/

        for (unsigned long i=0; i<HVO_params::time_steps; i++)
        {
            if (fmod(i, HVO_params::num_steps_per_beat*HVO_params::num_beats_per_bar) == 0)      // bar position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, bar_c, i, GroovePianoRollWidget2GrooveThreadQues));
            }
            else if(fmod(i, HVO_params::num_steps_per_beat) == 0)                  // beat position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, beat_c, i, GroovePianoRollWidget2GrooveThreadQues));
            }
            else                                                    // every other position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, def_c, i, GroovePianoRollWidget2GrooveThreadQues));
            }

            addAndMakeVisible(interactivePRollBlocks[i].get());
        }


    }

    // location must be between 0 or 1
    void addEventToStep(int idx, int hit_, float velocity_, float offset_)
    {
        interactivePRollBlocks[idx]->addEvent(hit_, velocity_, offset_);
    }


    void resized() override {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width)));
        auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
        for (int i = 0; i<HVO_params::time_steps; i++)
        {
            interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
        }
    }


};

/**
 * wraps two instances of PianoRoll_InteractiveMonotonicGroove together on top of each other.
 * Bottom one is unModified groove (interactive) and top one is Modified groove (non-interactive)
 * pianorolls.
 */
class MonotonicGrooveWidget:public juce::Component
{
public:
    unique_ptr<PianoRoll_InteractiveMonotonicGroove> unModifiedGrooveGui;
    unique_ptr<PianoRoll_InteractiveMonotonicGroove> ModifiedGrooveGui;

    MonotonicGrooveWidget(GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQues = nullptr)
    {
        // Create Unmodified Piano ROll
        unModifiedGrooveGui = make_unique<PianoRoll_InteractiveMonotonicGroove>(true, "Unmodified Groove", GroovePianoRollWidget2GrooveThreadQues);
        addAndMakeVisible(unModifiedGrooveGui.get());
        // Create Unmodified Piano ROll
        ModifiedGrooveGui = make_unique<PianoRoll_InteractiveMonotonicGroove>(false, "Adjusted Groove");
        addAndMakeVisible(ModifiedGrooveGui.get());

    }

    void resized() override {
        auto area = getLocalBounds();
        auto height = int((float) area.getHeight() *0.45f);
        ModifiedGrooveGui->setBounds(area.removeFromTop(height));
        unModifiedGrooveGui->setBounds(area.removeFromBottom(height));

    }

    void updateWithNewGroove(MonotonicGroove<HVO_params::time_steps> new_groove)
    {
        for (int i = 0; i < HVO_params::time_steps; i++)
        {
            unModifiedGrooveGui->interactivePRollBlocks[(size_t) i]->addEvent(
                new_groove.hvo.hits[i].item().toInt(),
                new_groove.hvo.velocities_unmodified[i].item().toFloat(),
                new_groove.hvo.offsets_unmodified[i].item().toFloat());

            ModifiedGrooveGui->interactivePRollBlocks[(size_t) i]->addEvent(
                new_groove.hvo.hits[i].item().toInt(),
                new_groove.hvo.velocities_modified[i].item().toFloat(),
                new_groove.hvo.offsets_modified[i].item().toFloat());
        }
    }

};