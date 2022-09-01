//
// Created by Behzad Haki on 2022-08-19.
//

#pragma once

#include "CustomUIWidgets.h"
#include "../settings.h"
#include "../Includes/CustomStructsAndLockFreeQueue.h"

using namespace std;

/***
 * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
 * row to represent the piano roll for a single drum voice. Also, a SingleStepPianoRollBlock::XYPadWithtListeners is used
 * to interact with the voice sampling/max number of generations allowed.
 */
class PianoRoll_GeneratedDrums_SingleVoice :public juce::Component
{
public:

    vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlockWithProbability>> interactivePRollBlocks;
    shared_ptr<SingleStepPianoRollBlock::XYPadAutomatableWithSliders> MaxCount_Prob_XYPad; // x axis will be Max count (0 to time_steps), y axis is threshold 0 to 1
    juce::Label label;
    int pianoRollSectionWidth {0};


    PianoRoll_GeneratedDrums_SingleVoice(juce::AudioProcessorValueTreeState* apvtsPntr, string label_text, string maxCountParamID, string threshParamID)
    {

        // Set Modified Label
        label.setText(label_text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible(label);

        // xy slider
        MaxCount_Prob_XYPad = make_shared<SingleStepPianoRollBlock::XYPadAutomatableWithSliders>(apvtsPntr, maxCountParamID, threshParamID);
        addAndMakeVisible(MaxCount_Prob_XYPad.get());

        // Draw up piano roll
        for (unsigned long i=0; i<HVO_params::time_steps; i++)
        {
            if (fmod(i, HVO_params::num_steps_per_beat*HVO_params::num_beats_per_bar) == 0)      // bar position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, bar_backg_color, i));
            }
            else if(fmod(i, HVO_params::num_steps_per_beat) == 0)                  // beat position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, beat_backg_color, i));
            }
            else                                                    // every other position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, rest_backg_color, i));
            }

            MaxCount_Prob_XYPad->addWidget(interactivePRollBlocks[i].get());
            addAndMakeVisible(interactivePRollBlocks[i].get());
        }

    }

    int getPianoRollSectionWidth()
    {
        return pianoRollSectionWidth;
    }

    int getPianoRollLeftBound()
    {
        return label.getWidth();
    }

    void addEventToTimeStep(int time_step_ix, int hit_, float velocity_, float offset_, float probability_)
    {
        interactivePRollBlocks[time_step_ix]->addEvent(hit_, velocity_, offset_, probability_, MaxCount_Prob_XYPad->ySlider.getValue());
    }

    void resized() override {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width)));
        auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
        pianoRollSectionWidth = 0;
        for (int i = 0; i<HVO_params::time_steps; i++)
        {
            interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
            pianoRollSectionWidth += interactivePRollBlocks[i]->getWidth();
        }
        MaxCount_Prob_XYPad->setBounds (area.removeFromBottom(proportionOfHeight(gui_settings::PianoRolls::prob_to_pianoRoll_Ratio)));
    }

};


/***
 * num_voices of PianoRoll_GeneratedDrums_SingleVoice packed on top of each other to represent the entire pianoRoll
 */
class GeneratedDrumsWidget :public juce::Component
{
public:
    vector<unique_ptr<PianoRoll_GeneratedDrums_SingleVoice>> PianoRolls;

    GeneratedDrumsWidget(juce::AudioProcessorValueTreeState* apvtsPntr)
    {
        auto DrumVoiceNames_ = nine_voice_kit_labels;
        auto DrumVoiceMidiNumbers_ = nine_voice_kit_default_midi_numbers;

        for (size_t voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
        {
            auto voice_label = DrumVoiceNames_[voice_i];
            auto label_txt = voice_label + "\n[Midi "+to_string(DrumVoiceMidiNumbers_[voice_i])+"]";

            PianoRolls.push_back(make_unique<PianoRoll_GeneratedDrums_SingleVoice>(
                apvtsPntr, label_txt, voice_label+"_X", voice_label+"_Y"));

            addAndMakeVisible(PianoRolls[voice_i].get());
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        int PRollheight = (int((float) area.getHeight() )) / HVO_params::num_voices;
        for (int voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
        {
            PianoRolls[voice_i]->setBounds(area.removeFromBottom(PRollheight));
        }
    }

    void addEventToVoice(int voice_number, int timestep_idx, int hit_, float velocity_, float offset, float probability_)
    {
        // add note
        PianoRolls[voice_number]->addEventToTimeStep(timestep_idx, hit_, velocity_, offset, probability_);

    }

    void updateWithNewScore(HVOLight <HVO_params::time_steps, HVO_params::num_voices> latest_generated_data)
    {

        for (int vn_= 0; vn_ < latest_generated_data.num_voices; vn_++)
        {
            for (int t_= 0; t_ < latest_generated_data.time_steps; t_++)
            {
                addEventToVoice(
                    vn_,
                    t_,
                    latest_generated_data.hits[t_][vn_].item().toInt(),
                    latest_generated_data.velocities[t_][vn_].item().toFloat(),
                    latest_generated_data.offsets[t_][vn_].item().toFloat(),
                    latest_generated_data.hit_probabilities[t_][vn_].item().toFloat());
            }

        }


        old_generated_data = latest_generated_data;
    }

    void UpdatePlayheadLocation (double playhead_percentage_)
    {
        playhead_percentage = playhead_percentage_;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto w = PianoRolls[0]->getPianoRollSectionWidth();
        auto x0 = PianoRolls[0]->getPianoRollLeftBound();
        auto x = w * playhead_percentage + x0;
        g.setColour(playback_progressbar_color);
        g.drawLine(x, 0, x, getHeight());
    }
private:
    HVOLight <HVO_params::time_steps, HVO_params::num_voices> old_generated_data{};
    double playhead_percentage {0};
};
