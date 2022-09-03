//
// Created by Behzad Haki on 2022-08-20.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../InterThreadFifos.h"


// ============================================================================================================
// ==========              Building Blocks of each of the time steps in the pianorolls            =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ==========
// ============================================================================================================
namespace SingleStepPianoRollBlock
{

    /***
     * (A) The smallest block to draw a drum event inside
     * If interactive, notes can be created by double clicking, moved around by dragging ...
     * Notes are drawn using offset values and velocity given a offset range (specified in settings.h)
     * Velocity range is from 0 to 1
     *
     * (B) if a queue is provided (of type LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>),
     * moving a note manually sends the new vel, hit, offset values to a receiving thread
     * (in this case, GrooveThread in processor)
     *
     * (C) InteractiveInstance is used for Monotonic Groove Visualization only as generated drums are not
     * manually modifiable
     */
    class InteractiveIndividualBlock : public juce::Component
    {
    public:
        bool isClickable;
        int hit;
        float velocity;
        float offset;
        juce::Colour backgroundcolor;
        int grid_index; // time step corresponding to the block
        float low_offset = min(HVO_params::_min_offset, HVO_params::_max_offset); // min func just incase min and max offsets are wrongly defined
        float hi_offset = max(HVO_params::_min_offset, HVO_params::_max_offset); // max func just incase min and max offsets are wrongly defined
        float range_offset = hi_offset - low_offset;
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue;

        /**
         * Constructor
         *
         * @param isClickable_ (bool) True for interactive version
         * @param backgroundcolor_  (juce::Colour type)
         * @param grid_index_ (int) specifies which time step the block is used for
         * @param GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr  (LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>*) used to send data to a receiver via this queue if interactive and also queue is not nullptr
         *
         */
        InteractiveIndividualBlock(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_,
                                   LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr = nullptr) {
            grid_index = grid_index_;
            backgroundcolor = backgroundcolor_;
            isClickable = isClickable_;
            GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr;
            hit = 0;
            velocity = 0;
            offset = 0;
        }

        void paint(juce::Graphics& g) override
        {

            //background colour
            g.fillAll(backgroundcolor);

            // get component dimensions
            auto w = (float) getWidth();
            auto h = (float) getHeight();

            // draw a line on the left side
            g.setColour(juce::Colours::black);
            juce::Point<float> p1_edge {0, h};
            juce::Point<float> p2_edge {0, 0};
            g.drawLine ({p1_edge, p2_edge}, 2.0f);

            if (hit == 1)
            {
                // draw a colored line and a rectangle on the top end for the note hit
                g.setColour(note_color);

                auto x_pos = offsetToActualX();
                auto y_pos = (float)proportionOfHeight(1-velocity);
                juce::Point<float> p1 {x_pos, h};
                juce::Point<float> p2 {x_pos, y_pos};
                auto thickness = 2.0f;
                g.drawLine ({p1, p2}, thickness);

                juce::Rectangle<float> rect {p2 , juce::Point<float> {x_pos + w * 0.3f, y_pos + w * 0.3f}};

                g.fillRect(rect);
                g.drawRect(x_pos, y_pos , w * 0.3f, w * 0.3f, thickness );
                // g.drawLine(x_pos, getHeight(),x_pos, y_pos);
            }

        }

        // only sendDataToQueue() if instance is interactive
        // only sends data when mouse key is released
        void mouseUp(const juce::MouseEvent& ) override
        {
            if (isClickable)
            {
                sendDataToQueue();
            }
        }

        // moves a note around the block on dragging and repaints
        void mouseDrag(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                if (hit == 1)
                {
                    // if shift pressed moves only up or down
                    if (!ev.mods.isShiftDown())
                    {
                        offset = actualXToOffset(ev.position.getX());
                    }
                    velocity = 1 - ev.position.getY() / (float) getHeight();
                }
                repaint();
            }
        }

        // hides an already existing note or unhides if the block is empty
        void mouseDoubleClick(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                if (hit == 0) // convert to hit
                {
                    if (velocity == 0) // if no previous note here! make a new one with offset 0
                    {
                        hit = 1;
                        velocity = 1.0f - ev.position.getY()/float(getHeight());
                        offset = 0;
                    }
                    else            // if note already here, double click activates it again
                    {
                        hit = 1;
                    }
                }
                else  // convert to silence without deleting velocity/offset info
                {
                    hit = 0;
                }
                sendDataToQueue();
                repaint();
            }
        }

        // places a note in the block at the given locations
        void addEvent(int hit_, float velocity_, float offset_)
        {
            assert (hit == 0 or hit == 1);

            hit = hit_;
            velocity = velocity_;
            offset = offset_;

            repaint();
        }

        // places a BasicNote in the queue to be received by another thread
        void sendDataToQueue() const
        {
            if (hit == 1)
            {
                GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->push(BasicNote(100, velocity, grid_index, offset));
            }
            else
            {
                GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->push(BasicNote(100, 0, grid_index, 0));
            }
        }

        // converts offset to actual x value on component
        float offsetToActualX()
        {
            float ratioOfWidth = (offset - low_offset) / range_offset;
            return (float)proportionOfWidth(ratioOfWidth);
        }

        // converts offset to actual x value on component
        float actualXToOffset(float x_pixel)
        {
            float ratioOfWidth = x_pixel / (float)getWidth();
            return (ratioOfWidth * range_offset + low_offset);
        }
    };


    /**
     *  The block in which probability levels are shown
     *  if probabilities are over 0, a curved peak is drawn at the centre
     *  with height corresponding to the specified probability level
     */
    class ProbabilityLevelWidget: public juce::Component
    {
    public:
        juce::Colour backgroundcolor;
        float hit_prob = 0;
        float sampling_threshold = 0;
        int hit = 0;

        /***
         * Constructor
         * @param backgroundcolor_ (juce:: Colour)

         */
        ProbabilityLevelWidget(juce:: Colour backgroundcolor_)
        {
            backgroundcolor = backgroundcolor_;
        }

        // draws a path peaking at the probability level
        void paint(juce::Graphics& g) override
        {
            //background colour
            g.fillAll(backgroundcolor);

            auto h = (float) getHeight();
            auto w = (float) getWidth();

            g.setColour(juce::Colours::white);
            auto y_= (1 - sampling_threshold) * h;
            g.drawLine(-1.0f, y_, h*2.0f, y_);

            juce::Path myPath1;
            float from_edge = 0.4f;
            auto p = (float) proportionOfHeight(1.0f - hit_prob); // location of peak (ie probability)
            auto control_rect = juce::Rectangle<float> (juce::Point<float> ((float) proportionOfWidth(from_edge), h), juce::Point<float> ((float)proportionOfWidth(1.0f - from_edge), p));

            auto half_P = (float) proportionOfHeight(1.0f - hit_prob/2.0f);
            g.setColour(prob_color_non_hit);
            myPath1.startNewSubPath (0.0f, h);

            myPath1.lineTo(juce::Point<float> (w, h));
            g.strokePath (myPath1, juce::PathStrokeType (2.0f));

            // draw a quadratic curve if probability is over 0
            if (hit_prob > 0)
            {
                juce::Path myPath;
                myPath.startNewSubPath (0.0f, h);

                auto fillType = juce::FillType();
                fillType.setColour(prob_color_non_hit);

                if (hit == 1 )
                {
                    g.setColour(prob_color_hit);
                    fillType.setColour(prob_color_hit);
                }

                myPath.quadraticTo(control_rect.getBottomLeft(), juce::Point<float> ((float)proportionOfWidth(from_edge), half_P));
                myPath.quadraticTo(control_rect.getTopLeft(), juce::Point<float> ((float)proportionOfWidth(0.5f), p));
                myPath.quadraticTo(control_rect.getTopRight(), juce::Point<float> ((float)proportionOfWidth(1.0f - from_edge), half_P));
                myPath.quadraticTo(control_rect.getBottomRight(), juce::Point<float> (w, h));
                myPath.closeSubPath();

                g.setFillType(fillType);
                g.fillPath(myPath);
            }
        }

        /** Specify probability state,
         * i.e. what the probability level and sampling thresholds are
         * and also whether the note will actually get played (if the note is
         * one of the n_max most probable events with prob over threshold)
         * @param hit_ (int) if 1, then the color for peak will be darker
         * @param hit_prob_  (float) probability level
         * @param sampling_thresh (float) line level used for sampling threshold
         */
        void setProbability(int hit_, float hit_prob_, float sampling_thresh)
        {
            hit = hit_;
            hit_prob = hit_prob_;
            sampling_threshold = sampling_thresh;
            repaint();
        }

        void setSamplingThreshold(float thresh)
        {
            sampling_threshold = thresh;
            repaint();
        }


    };


    /**
     * Wraps a PianoRoll_InteractiveIndividualBlock and ProbabilityLevelWidget together into one component
     */
    class InteractiveIndividualBlockWithProbability : public juce::Component
    {

    public:
        unique_ptr<InteractiveIndividualBlock> pianoRollBlockWidgetPntr;  // unique_ptr to allow for initialization in the constructor
        unique_ptr<ProbabilityLevelWidget> probabilityCurveWidgetPntr;         // component instance within which we'll draw the probability curve

        InteractiveIndividualBlockWithProbability(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_,
                                                  LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQues=nullptr)
        {
            pianoRollBlockWidgetPntr = make_unique<InteractiveIndividualBlock>(isClickable_, backgroundcolor_, grid_index_, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQues);
            addAndMakeVisible(pianoRollBlockWidgetPntr.get());


            probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(juce::Colours::black);
            addAndMakeVisible(probabilityCurveWidgetPntr.get());
        }

        void addEvent(int hit_, float velocity_, float offset, float hit_prob_, float sampling_threshold) const
        {
            if (pianoRollBlockWidgetPntr->hit != hit_ or pianoRollBlockWidgetPntr->velocity != velocity_ or
                pianoRollBlockWidgetPntr->offset != offset)
            {
                pianoRollBlockWidgetPntr->addEvent(hit_, velocity_, offset);
            }

            probabilityCurveWidgetPntr->setProbability(hit_, hit_prob_, sampling_threshold);

        }

        void resized() override
        {
            auto area = getLocalBounds();
            auto prob_to_pianoRoll_Ratio = 0.4f;
            auto h = (float) area.getHeight();
            pianoRollBlockWidgetPntr->setBounds (area.removeFromTop(int((1-prob_to_pianoRoll_Ratio)*h)));
            probabilityCurveWidgetPntr->setBounds (area.removeFromBottom(int(h*prob_to_pianoRoll_Ratio)));
        }
    };



    class XYPadAutomatableWithSliders: public juce::Component, public juce::Slider::Listener
    {
    public:
        vector<InteractiveIndividualBlockWithProbability*> ListenerWidgets;

        juce::Slider xSlider;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xSliderAttachement;
        juce::Slider ySlider;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ySliderAttachement;
        juce::AudioProcessorValueTreeState* apvts;

        float latest_x = 0;
        float latest_y = 0;

        XYPadAutomatableWithSliders(juce::AudioProcessorValueTreeState* apvtsPntr,
                                    const juce::String xParameterID , const juce::String yParameterID)
        {
            apvts = apvtsPntr;
            xSliderAttachement = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvts, xParameterID, xSlider);
            ySliderAttachement = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvts, yParameterID, ySlider);
        }

        void sliderValueChanged (juce::Slider* slider) override
        {
            if (slider == &xSlider or slider == &ySlider)
            {
                repaint();
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
            g.fillAll(juce::Colours::black);
            g.setColour(juce::Colours::white);

            auto w = float(getWidth());
            auto h = float(getHeight());

            auto x_ = round(max(min(float(xSlider.getValue() - xSlider.getMinimum()) * w / float(xSlider.getMaximum() - xSlider.getMinimum()), w), 0.0f));
            auto y_ = max(min(float(1.0f - (ySlider.getValue() - ySlider.getMinimum()) / float(ySlider.getMaximum() - ySlider.getMinimum())) * h, h), 0.0f);


            // g.setColour(juce::Colours::beige);
            g.setColour(prob_color_hit);
            juce::Point<float> p1 {0, 0};
            juce::Point<float> p2 {x_, y_};
            juce::Rectangle<float> rect {p1, p2};
            g.fillRect(rect);
            g.drawRect(rect, 0.2f);
            g.setColour(juce::Colours::white);
            g.drawLine(0.0f, y_, x_, y_, 2);
            g.drawLine(x_, 0, x_, y_, 2);

        }

        void mouseDoubleClick(const juce::MouseEvent&) override
        {
            // more than 2 clicks goes back to default
            if (xSlider.getValue() == 0 and ySlider.getValue() == 1)
            {
                xSlider.setValue(latest_x);
                ySlider.setValue(latest_y);
            }
            else // regular double click sets area to 0 --> no possible hits
            {
                latest_x = (float) xSlider.getValue();
                latest_y = (float) ySlider.getValue();
                xSlider.setValue(0.0f);
                ySlider.setValue(1.0f);
            }
            repaint();
            BroadCastThresholds();
        }


        void mouseDrag(const juce::MouseEvent& ev) override
        {
            m_mousepos = ev.position;
            xSlider.setValue(m_mousepos.getX()/float(getWidth()) * (xSlider.getMaximum() - xSlider.getMinimum())+ xSlider.getMinimum());
            ySlider.setValue((1.0f - m_mousepos.getY()/float(getHeight())) * (ySlider.getMaximum() - ySlider.getMinimum())+ ySlider.getMinimum());
            repaint();
            BroadCastThresholds();
        }

        /**
         * Adds PianoRoll_InteractiveIndividualBlockWithProbability instances to an internal vector.
         * THis way sampling thresholds in the "listener" widgets can be automatically updated
         * @param widget
         */
        void addWidget(InteractiveIndividualBlockWithProbability* widget)
        {
            ListenerWidgets.push_back(widget);
            BroadCastThresholds();
        }

        // sends sampling thresholds to listener widgets
        void BroadCastThresholds()
        {
            // at least one widget should have been added using
            // addWidget(). If no listeners, you should use the basic
            // UI::XYPad component
            if (!ListenerWidgets.empty())
            {
                auto thresh = ySlider.getValue();

                for (size_t i=0; i<ListenerWidgets.size(); i++)
                {
                    ListenerWidgets[i]->probabilityCurveWidgetPntr->setSamplingThreshold(float(thresh));
                }
            }

        }


    private:
        juce::Point<float> m_mousepos;

    };
}


// ============================================================================================================
// ==========              UI WIDGETS PLACED ON FINAL EDITOR GUI                                  =============
// ==========
// ============================================================================================================
namespace FinalUIWidgets {

    namespace GeneratedDrums
    {
        /***
         * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
         * row to represent the piano roll for a single drum voice. Also, a SingleStepPianoRollBlock::XYPadWithtListeners is used
         * to interact with the voice sampling/max number of generations allowed.
         */
        class GeneratedDrums_SingleVoice :public juce::Component
        {
        public:

            vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlockWithProbability>> interactivePRollBlocks;
            shared_ptr<SingleStepPianoRollBlock::XYPadAutomatableWithSliders> MaxCount_Prob_XYPad; // x-axis will be Max count (0 to time_steps), y-axis is threshold 0 to 1
            juce::Slider midiNumberSlider;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  midiNumberSliderAttachment;
            juce::Label label;
            int pianoRollSectionWidth {0};

            GeneratedDrums_SingleVoice(juce::AudioProcessorValueTreeState* apvtsPntr, const string label_text, const string maxCountParamID, const string threshParamID, const string midiNumberParamID)
            {
                // attach midiNumberSlider to apvts
                midiNumberSliderAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, midiNumberParamID, midiNumberSlider);
                addAndMakeVisible(midiNumberSlider);

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
                interactivePRollBlocks[(size_t)time_step_ix]->addEvent(hit_, velocity_, offset_, probability_, (float)MaxCount_Prob_XYPad->ySlider.getValue());
            }

            void resized() override {
                auto area = getLocalBounds();
                area.removeFromRight(area.getWidth() - (int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
                label.setBounds(area.removeFromTop(area.proportionOfHeight(0.5f)));
                auto h = area.getHeight();
                midiNumberSlider.setBounds(area);
                midiNumberSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, midiNumberSlider.getWidth()/4, int(h/3.0));

                // place pianorolls for each step
                area = getLocalBounds();
                area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
                auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
                pianoRollSectionWidth = 0;
                for (size_t i = 0; i<HVO_params::time_steps; i++)
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
            vector<unique_ptr<GeneratedDrums_SingleVoice>> PianoRolls;

            GeneratedDrumsWidget(juce::AudioProcessorValueTreeState* apvtsPntr)
            {
                auto DrumVoiceNames_ = nine_voice_kit_labels;
                auto DrumVoiceMidiNumbers_ = nine_voice_kit_default_midi_numbers;

                for (size_t voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
                {
                    auto voice_label = DrumVoiceNames_[voice_i];
                    auto label_txt = voice_label;

                    PianoRolls.push_back(make_unique<GeneratedDrums_SingleVoice>(
                        apvtsPntr, label_txt, voice_label+"_X", voice_label+"_Y", voice_label+"_MIDI"));

                    addAndMakeVisible(PianoRolls[voice_i].get());
                }
            }

            void resized() override {
                auto area = getLocalBounds();
                int PRollheight = (int((float) area.getHeight() )) / HVO_params::num_voices;
                for (size_t voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
                {
                    PianoRolls[voice_i]->setBounds(area.removeFromBottom(PRollheight));
                }
            }

            void addEventToVoice(int voice_number, int timestep_idx, int hit_, float velocity_, float offset, float probability_)
            {
                // add note
                PianoRolls[(size_t) voice_number]->addEventToTimeStep(timestep_idx, hit_, velocity_, offset, probability_);

            }

            void updateWithNewScore(const HVOLight <HVO_params::time_steps, HVO_params::num_voices> latest_generated_data)
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
                auto x = float (w * playhead_percentage + x0);
                g.setColour(playback_progressbar_color);
                g.drawLine(x, 0, x, float(getHeight()));
            }
        private:
            HVOLight <HVO_params::time_steps, HVO_params::num_voices> old_generated_data{};
            double playhead_percentage {0};
        };

    }

    namespace MonotonicGrooves
    {
        /***
     * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
     * row to represent either the piano roll for the unmodified (interactive) OR the modified (non-interactive) sections of
     * of the piano roll for the Input Groove.
     */
        class InteractiveMonotonicGrooveSingleRow :public juce::Component
        {
        public:

            vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlock>> interactivePRollBlocks;
            juce::Colour def_c {juce::Colour::fromFloatRGBA(1.0f,1.0f,1.0f,0.8f)};
            juce::Colour beat_c { juce::Colour::fromFloatRGBA(.75f,.75f,.75f, 0.5f)};
            juce::Colour bar_c {  juce::Colour::fromFloatRGBA(.6f,.6f,.6f, 0.5f) };
            juce::Label label;

            InteractiveMonotonicGrooveSingleRow(bool isInteractive, const string label_text,
                                                LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = nullptr)
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
                        interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, bar_c, i, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue));
                    }
                    else if(fmod(i, HVO_params::num_steps_per_beat) == 0)                  // beat position
                    {
                        interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, beat_c, i, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue));
                    }
                    else                                                    // every other position
                    {
                        interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, def_c, i, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue));
                    }

                    addAndMakeVisible(interactivePRollBlocks[i].get());
                }


            }

            // location must be between 0 or 1
            void addEventToStep(int idx, int hit_, float velocity_, float offset_)
            {
                interactivePRollBlocks[(size_t) idx]->addEvent(hit_, velocity_, offset_);
            }


            void resized() override {
                auto area = getLocalBounds();
                label.setBounds(area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width)));
                auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
                for (size_t i = 0; i<HVO_params::time_steps; i++)
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
            unique_ptr<InteractiveMonotonicGrooveSingleRow> unModifiedGrooveGui;
            unique_ptr<InteractiveMonotonicGrooveSingleRow> ModifiedGrooveGui;

            MonotonicGrooveWidget(LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = nullptr)
            {
                // Create Unmodified Piano ROll
                unModifiedGrooveGui = make_unique<InteractiveMonotonicGrooveSingleRow>(true, "Unmodified Groove", GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue);
                addAndMakeVisible(unModifiedGrooveGui.get());
                // Create Unmodified Piano ROll
                ModifiedGrooveGui = make_unique<InteractiveMonotonicGrooveSingleRow>(false, "Adjusted Groove");
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


        class GrooveControlSliders: public juce::Component
        {
        public:
            // sliders for groove manipulation
            juce::Slider minVelSlider;
            juce::Label  minVelLabel;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minVelSliderAPVTSAttacher;
            juce::Slider maxVelSlider;
            juce::Label  maxVelLabel;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxVelSliderAPVTSAttacher;
            juce::Slider minOffsetSlider;
            juce::Label  minOffsetLabel;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minOffsetSliderAPVTSAttacher;
            juce::Slider maxOffsetSlider;
            juce::Label  maxOffsetLabel;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxOffsetSliderAPVTSAttacher;


            GrooveControlSliders(juce::AudioProcessorValueTreeState* apvtsPntr)
            {
                // sliders for vel offset ranges
                addAndMakeVisible (minVelSlider);
                minVelLabel.setText ("Min Vel", juce::dontSendNotification);
                addAndMakeVisible (minVelLabel);
                minVelSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "MIN_VELOCITY", minVelSlider);

                addAndMakeVisible (maxVelSlider);
                addAndMakeVisible (maxVelLabel);
                maxVelLabel.setText ("Max Vel", juce::dontSendNotification);
                maxVelSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "MAX_VELOCITY", maxVelSlider);

                addAndMakeVisible (minOffsetSlider);
                minOffsetSlider.setRange (HVO_params::_min_offset, HVO_params::_max_offset);
                addAndMakeVisible (minOffsetLabel);
                minOffsetLabel.setText ("Min Offset", juce::dontSendNotification);
                minOffsetSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "MIN_OFFSET", minOffsetSlider);


                addAndMakeVisible (maxOffsetSlider);
                maxOffsetSlider.setRange (HVO_params::_min_offset, HVO_params::_max_offset);
                addAndMakeVisible (maxOffsetLabel);
                maxOffsetLabel.setText ("Max Offset", juce::dontSendNotification);
                maxOffsetSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "MAX_OFFSET", maxOffsetSlider);
            }

            void resized() override
            {
                // put vel offset range sliders
                {
                    auto area = getLocalBounds();
                    area.removeFromLeft(area.proportionOfWidth(0.7f));
                    auto height = area.proportionOfHeight(0.25f);
                    minVelLabel.setBounds(area.removeFromTop(height));
                    maxVelLabel.setBounds(area.removeFromTop(height));
                    minOffsetLabel.setBounds(area.removeFromTop(height));
                    maxOffsetLabel.setBounds(area.removeFromTop(height));
                }

                {
                    auto area = getLocalBounds();
                    area.removeFromRight(area.proportionOfWidth(0.3f));
                    auto height = area.proportionOfHeight(0.25f);
                    minVelSlider.setBounds(area.removeFromTop(height));
                    maxVelSlider.setBounds(area.removeFromTop(height));
                    minOffsetSlider.setBounds(area.removeFromTop(height));
                    maxOffsetSlider.setBounds(area.removeFromTop(height));
                }
            }

        };
    }

    namespace ResetButtons
    {

    }
}



