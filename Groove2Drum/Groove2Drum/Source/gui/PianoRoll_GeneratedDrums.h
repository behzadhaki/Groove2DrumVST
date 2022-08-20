//
// Created by Behzad Haki on 2022-08-19.
//

#pragma once

#include "PianoRoll_InteractiveIndividualBlock.h"


using namespace std;


class PianoRoll_GeneratedDrums_SingleVoice :public juce::Component
{
public:

    vector<shared_ptr<PianoRoll_InteractiveIndividualBlockWithProbability>> interactivePRollBlocks;
    shared_ptr<XYPlane> MaxCount_Prob_XYPlane; // x axis will be Max count (0 to time_steps), y axis is threshold 0 to 1
    juce::Label label;


    juce::Colour def_c {juce::Colour::fromFloatRGBA(1.0f,1.0f,1.0f,1.0f)};
    juce::Colour beat_c { juce::Colour::fromFloatRGBA(.75f,.75f,.75f,1.0f)};
    juce::Colour bar_c {  juce::Colour::fromFloatRGBA(.4f,.4f,.4f,1.0f) };

    int num_gridlines;
    float step_ppq;

    PianoRoll_GeneratedDrums_SingleVoice(int num_gridlines_, float step_ppq_, int n_steps_per_beat, int n_beats_per_bar, string label_text, int voice_number_=0)
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
                interactivePRollBlocks.push_back(make_shared<PianoRoll_InteractiveIndividualBlockWithProbability>(false, bar_c, i, voice_number_));
            }
            else if(fmod(i, n_steps_per_beat) == 0)                  // beat position
            {
                interactivePRollBlocks.push_back(make_shared<PianoRoll_InteractiveIndividualBlockWithProbability>(false, beat_c, i, voice_number_));
            }
            else                                                    // every other position
            {
                interactivePRollBlocks.push_back(make_shared<PianoRoll_InteractiveIndividualBlockWithProbability>(false, def_c, i, voice_number_));
            }

            addAndMakeVisible(interactivePRollBlocks[i].get());

        }

        MaxCount_Prob_XYPlane = make_shared<XYPlane>(0, num_gridlines_, num_gridlines_/2, 0, 1, 0.5);
        addAndMakeVisible(MaxCount_Prob_XYPlane.get());
    }

    // location must be between 0 or 1
    void addEventToStep(int idx, float location, int hit_, float velocity_, float probability_)
    {
        interactivePRollBlocks[idx]->addEvent(hit_, velocity_, location, probability_);
    }
    void addEventWithPPQ(float ppq_, int hit_, float velocity_, float probability_)
    {
        auto idx = (unsigned long) (floor(ppq_/step_ppq));
        interactivePRollBlocks[idx]->addEventWithPPQ(hit_, velocity_, ppq_, probability_, step_ppq);
    }

    void resized() override {
        auto area = getLocalBounds();
        auto w = (float) area.getWidth();
        auto h = (float) area.getHeight();

        // layout slider on lower right corner
        auto prob_to_pianoRoll_Ratio =  0.5f;
        area.removeFromLeft(int(w-h*prob_to_pianoRoll_Ratio));
        MaxCount_Prob_XYPlane->setBounds (area.removeFromBottom(int(h*prob_to_pianoRoll_Ratio)));

        // layout the rest
        area = getLocalBounds();
        w = (float) area.getWidth();
        h = (float) area.getHeight();
        area.removeFromRight(int(h*prob_to_pianoRoll_Ratio));

        w = (float) area.getWidth();
        h = (float) area.getHeight();
        auto label_ratio_of_width = 0.1f;
        auto label_width = (int) (label_ratio_of_width * w);
        label.setBounds(area.removeFromLeft(label_width));

        //MaxCount_Prob_XYPlane->setBounds(area.removeFromRight(h));

        auto grid_width = area.getWidth() / num_gridlines;
        for (int i = 0; i<num_gridlines; i++)
        {
            interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
        }

    }


};


class PianoRoll_GeneratedDrums_AllVoices:public juce::Component
{
public:
    vector<unique_ptr<PianoRoll_GeneratedDrums_SingleVoice>> PianoRoll;
    int num_voices;
    float step_ppq_duration;

    PianoRoll_GeneratedDrums_AllVoices(int num_gridlines_, float step_ppq_duration_, int n_steps_per_beat_, int n_beats_per_bar_, vector<string> DrumVoiceNames_, vector<int> DrumVoiceMidiNumbers_)
    {
        assert (DrumVoiceNames_.size()==DrumVoiceMidiNumbers_.size());

        num_voices = int(DrumVoiceMidiNumbers_.size());
        step_ppq_duration = step_ppq_duration_;

        for (int voice_i=0; voice_i<num_voices; voice_i++)
        {
            auto label_txt = DrumVoiceNames_[voice_i] + "\n[Midi "+to_string(DrumVoiceMidiNumbers_[voice_i])+"]";

            PianoRoll.push_back(make_unique<PianoRoll_GeneratedDrums_SingleVoice>(num_gridlines_, step_ppq_duration, n_steps_per_beat_, n_beats_per_bar_, label_txt, voice_i));
            addAndMakeVisible(PianoRoll[voice_i].get());
        }
        // Create Unmodified Piano ROll

    }

    void resized() override {
        auto total_gaps = int((float) getHeight() * 0.1f);
        auto area = getLocalBounds();
        int PRollheight;
        PRollheight = (int((float) area.getHeight() )- total_gaps) / num_voices;
        auto GapHeight = int (total_gaps / num_voices);
        for (int voice_i=0; voice_i<num_voices; voice_i++)
        {
            PianoRoll[voice_i]->setBounds(area.removeFromBottom(PRollheight));
            area.removeFromBottom(GapHeight);
        }
    }

    void addEventToStep(int voice_number, int grid_index, float location, int hit_, float velocity_, float probability_)
    {
        PianoRoll[voice_number]->interactivePRollBlocks[grid_index]->addEvent(hit_, velocity_, location, probability_);
    }

    void addEventWithPPQ(int voice_number, float ppq_, int hit_, float velocity_, float probability_)
    {
        auto idx = (unsigned long) (floor(ppq_/step_ppq_duration));
        PianoRoll[voice_number]->interactivePRollBlocks[idx]->addEventWithPPQ(hit_, velocity_, ppq_, probability_, step_ppq_duration);
    }

};
