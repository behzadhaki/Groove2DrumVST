//
// Created by Behzad Haki on 2022-08-19.
//

#pragma once

#include "CustomUIWidgets.h"
#include "../settings.h"
#include "../Includes/CustomStructsAndLockFreeQueue.h"

using namespace std;
using namespace UI;

/***
 * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
 * row to represent the piano roll for a single drum voice. Also, a SingleStepPianoRollBlock::XYPadWithtListeners is used
 * to interact with the voice sampling/max number of generations allowed.
 */
class PianoRoll_GeneratedDrums_SingleVoice :public juce::Component
{
public:

    vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlockWithProbability>> interactivePRollBlocks;
    shared_ptr<SingleStepPianoRollBlock::XYPadWithtListeners> MaxCount_Prob_XYPad; // x axis will be Max count (0 to time_steps), y axis is threshold 0 to 1
    juce::Label label;
    int pianoRollSectionWidth {0};

    int num_gridlines ;

    PianoRoll_GeneratedDrums_SingleVoice(LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* max_num_to_modelThread_que,
                                         LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* sample_thresh_to_modelThread_que,
                                         int num_gridlines_, float step_ppq_, int n_steps_per_beat, int n_beats_per_bar, string label_text, int voice_number_=0)
    {
        num_gridlines = num_gridlines_;


        // Set Modified Label
        label.setText(label_text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible(label);

        // xy slider broadcaster
        MaxCount_Prob_XYPad = make_shared<SingleStepPianoRollBlock::XYPadWithtListeners>(0, num_gridlines_, num_gridlines_/2, 0, 1, 0.5, max_num_to_modelThread_que, sample_thresh_to_modelThread_que);
        addAndMakeVisible(MaxCount_Prob_XYPad.get());

        // Draw up piano roll
        /*auto w_per_block = (int) size_width/num_gridlines;*/

        for (unsigned long i=0; i<HVO_params::time_steps; i++)
        {
            if (fmod(i, n_steps_per_beat*n_beats_per_bar) == 0)      // bar position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, bar_backg_color, i, voice_number_));
            }
            else if(fmod(i, n_steps_per_beat) == 0)                  // beat position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, beat_backg_color, i, voice_number_));
            }
            else                                                    // every other position
            {
                interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                    InteractiveIndividualBlockWithProbability>(false, rest_backg_color, i, voice_number_));
            }

            auto prob_widget_listener = interactivePRollBlocks[i]->probabilityCurveWidgetPntr.get(); // allow slider to update line in the probability widgets
            prob_widget_listener->setSamplingThreshold(MaxCount_Prob_XYPad->getYValue());         // synchronize thresh line with the defaul in Slider
            MaxCount_Prob_XYPad->addWidget(interactivePRollBlocks[i].get());
            addAndMakeVisible(interactivePRollBlocks[i].get());
        }

        // set initial defaul values of XYPad (MUST BE AFTER DEFINING AND ATTACHING THE PER STEP WIDGETS
        MaxCount_Prob_XYPad->updateDefaultValues(8, 0.5f);

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
        interactivePRollBlocks[time_step_ix]->addEvent(hit_, velocity_, offset_, probability_, MaxCount_Prob_XYPad->getYValue());
    }

    void resized() override {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width)));
        auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
        pianoRollSectionWidth = 0;
        for (int i = 0; i<num_gridlines; i++)
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
    vector<unique_ptr<PianoRoll_GeneratedDrums_SingleVoice>> PianoRoll;
    int num_voices;
    float step_ppq_duration;

    GeneratedDrumsWidget(int num_gridlines_, float step_ppq_duration_, int n_steps_per_beat_, int n_beats_per_bar_,
                                       vector<string> DrumVoiceNames_, vector<int> DrumVoiceMidiNumbers_,
                                       GuiIOFifos::DrumPianoRollWidgetToModelThreadQues* DrumPianoRollWidgetToModelThreadQuesPntr,
                                       std::vector<float> sampling_thresholds, std::vector<float> max_voices_allowed)
    {
        assert (DrumVoiceNames_.size()==DrumVoiceMidiNumbers_.size());

        num_voices = int(DrumVoiceMidiNumbers_.size());
        step_ppq_duration = step_ppq_duration_;

        for (size_t voice_i=0; voice_i<num_voices; voice_i++)
        {
            auto label_txt = DrumVoiceNames_[voice_i] + "\n[Midi "+to_string(DrumVoiceMidiNumbers_[voice_i])+"]";

            PianoRoll.push_back(make_unique<PianoRoll_GeneratedDrums_SingleVoice>(
                &DrumPianoRollWidgetToModelThreadQuesPntr->new_max_number_voices[voice_i],
                &DrumPianoRollWidgetToModelThreadQuesPntr->new_sampling_thresholds[voice_i],
                num_gridlines_, step_ppq_duration, n_steps_per_beat_, n_beats_per_bar_, label_txt, voice_i));
            PianoRoll[voice_i]->MaxCount_Prob_XYPad->updateDefaultValues(max_voices_allowed[voice_i], sampling_thresholds[voice_i]);
            addAndMakeVisible(PianoRoll[voice_i].get());
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        int PRollheight = (int((float) area.getHeight() )) / num_voices;
        for (int voice_i=0; voice_i<num_voices; voice_i++)
        {
            PianoRoll[voice_i]->setBounds(area.removeFromBottom(PRollheight));
        }
    }

    void addEventToVoice(int voice_number, int timestep_idx, int hit_, float velocity_, float offset, float probability_)
    {
        // add note
        PianoRoll[voice_number]->addEventToTimeStep(timestep_idx, hit_, velocity_, offset, probability_);

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
        auto w = PianoRoll[0]->getPianoRollSectionWidth();
        auto x0 = PianoRoll[0]->getPianoRollLeftBound();
        auto x = w * playhead_percentage + x0;
        g.setColour(playback_progressbar_color);
        g.drawLine(x, 0, x, getHeight());
    }
private:
    HVOLight <HVO_params::time_steps, HVO_params::num_voices> old_generated_data{};
    double playhead_percentage {0};
};
