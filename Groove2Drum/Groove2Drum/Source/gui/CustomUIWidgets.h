//
// Created by Behzad Haki on 2022-08-20.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../InterThreadFifos.h"

// ============================================================================================================
// ==========           General User Interface Components such as basic XYPad  Implementation     =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ==========
// ============================================================================================================

namespace UI
{
    class XYPad : public juce::Component
{
public:
    float x_min; float x_max; float x_default;
    float y_min; float y_max; float y_default;
    float x_ParameterValue; float y_ParameterValue; // actual values of x or y parameters within the min/max ranges specified (NOT in terms of pixels!)


    XYPad(float x_min_, float x_max_, float x_default_, float y_min_, float y_max_, float y_default_):
        x_min(x_min_), x_max(x_max_), x_default(x_default_),
        y_min(y_min_), y_max(y_max_), y_default(y_default_),
        x_ParameterValue(x_default_), y_ParameterValue(y_default_)
    {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);

        auto w = float(getWidth());
        auto h = float(getHeight());

        auto x_ = round(max(min((x_ParameterValue - x_min) * w / (x_max - x_min), w), 0.0f));
        auto y_ = max(min((1 - (y_ParameterValue - y_min) / (y_max - y_min)) * h, h), 0.0f);


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

    void moveToCoordinate(juce::Point<float> xyCoordinate)
    {
        auto w = float(getWidth());
        auto h = float(getHeight());

        float x_coor = round(max(min(xyCoordinate.getX(), w), 0.0f));
        float y_coor = max(min(xyCoordinate.getY(), h), 0.0f);

        x_ParameterValue = x_min + x_coor / w * (x_max - x_min);
        y_ParameterValue = y_min + (1 - y_coor / h) * (y_max - y_min);

        if (!(xyCoordinate.getX() == 0 and xyCoordinate.getY() == 0))
        {
            x_default = x_ParameterValue;
            y_default = y_ParameterValue;
        }
        repaint();
    }

    void moveUsingActualValues(float x_ParameterValue_, float y_ParameterValue_)
    {
        x_ParameterValue = x_ParameterValue_;
        y_ParameterValue = y_ParameterValue_;

        repaint();
    }

    void shareParameter()
    {
        // implement the process for sharing data with other listeners
        // parameter should be updated
        // DBG("Implement Transfer to update to X " << x_ParameterValue << " ,Y "<<y_ParameterValue);
    }

    void mouseUp(const juce::MouseEvent& ev) override
    {
        shareParameter();
    }

    /*void mouseDown(const juce::MouseEvent& ev) override
    {
        m_mousepos = ev.position;
        moveToCoordinate(m_mousepos);
    }*/

    void mouseDoubleClick(const juce::MouseEvent& ev) override
    {
        // more than 2 clicks goes back to default
        if (x_ParameterValue == 0 and y_ParameterValue == 1)
        {
            // shift with double click sets area to max
            if (ev.mods.isShiftDown())
            {
                moveToCoordinate(juce::Point<float> {(float)getX(), (float)getY()});
            }
            else
            {
                DBG(x_default);
                DBG(y_default);
                moveUsingActualValues(x_default, y_default);
            }

        }
        else // regular double click sets area to 0 --> no possible hits
        {
            moveToCoordinate(juce::Point<float> {0.0f, 0.0f});
        }


        shareParameter();
    }

    void mouseDrag(const juce::MouseEvent& ev) override
    {
        m_mousepos = ev.position;
        moveToCoordinate(m_mousepos);
    }

    float getXValue() const
    {
        return x_ParameterValue;
    }

    float getYValue() const
    {
        return y_ParameterValue;
    }

    void updateDefaultValues(float default_x_, float default_y_)
    {
        x_default = default_x_;
        y_default = default_y_;
        shareParameter();
        repaint();
    }

private:
    juce::Point<float> m_mousepos;
};
}




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
     * (B) if a queue is provided (of type GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues, see InterThreadFifos.h),
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
        int voice_num;
        float low_offset = min(HVO_params::_min_offset, HVO_params::_max_offset); // min func just incase min and max offsets are wrongly defined
        float hi_offset = max(HVO_params::_min_offset, HVO_params::_max_offset); // max func just incase min and max offsets are wrongly defined
        float range_offset = hi_offset - low_offset;
        GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQues;

        /**
         * Constructor
         *
         * @param isClickable_ (bool) True for interactive version
         * @param backgroundcolor_  (juce::Colour type)
         * @param grid_index_ (int) specifies which time step the block is used for
         * @param voice_num_ (int) specifies which drum voice the block is used for
         * @param GroovePianoRollWidget2GrooveThreadQuesPntr  (GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues, see InterThreadFifos.h) used to send data to a receiver via this queue if interactive and also queue is not nullptr
         *
         */
        InteractiveIndividualBlock(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_, int voice_num_ = 0, GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQuesPntr = nullptr) {
            grid_index = grid_index_;
            backgroundcolor = backgroundcolor_;
            isClickable = isClickable_;
            GroovePianoRollWidget2GrooveThreadQues = GroovePianoRollWidget2GrooveThreadQuesPntr;
            hit = 0;
            velocity = 0;
            offset = 0;
            voice_num = voice_num_;
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
        void mouseUp(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                sendDataToQueue();
            }
        }

        // moves a note around the block on dragging and repaints
        void mouseDrag(const juce::MouseEvent& ev)
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
        void sendDataToQueue()
        {
            if (hit == 1)
            {
                GroovePianoRollWidget2GrooveThreadQues->manually_drawn_notes.push(BasicNote(voice_num, velocity, grid_index, offset));
            }
            else
            {
                GroovePianoRollWidget2GrooveThreadQues->manually_drawn_notes.push(BasicNote(voice_num, 0, grid_index, 0));
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
        int hit;

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
        unique_ptr<InteractiveIndividualBlock> pianoRollBlockWidgetPntr;  // unique_ptr so as to allow for initialization in the constructor
        unique_ptr<ProbabilityLevelWidget> probabilityCurveWidgetPntr;         // component instance within which we'll draw the probability curve

        InteractiveIndividualBlockWithProbability(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_, int voice_num_, GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQues=nullptr)
        {
            pianoRollBlockWidgetPntr = make_unique<InteractiveIndividualBlock>(isClickable_, backgroundcolor_, grid_index_, voice_num_, GroovePianoRollWidget2GrooveThreadQues);
            addAndMakeVisible(pianoRollBlockWidgetPntr.get());


            probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(juce::Colours::black);
            addAndMakeVisible(probabilityCurveWidgetPntr.get());
        }

        void addEvent(int hit_, float velocity_, float offset, float hit_prob_, float sampling_threshold)
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
            auto w = (float) area.getWidth();
            auto h = (float) area.getHeight();
            pianoRollBlockWidgetPntr->setBounds (area.removeFromTop(int((1-prob_to_pianoRoll_Ratio)*h)));
            probabilityCurveWidgetPntr->setBounds (area.removeFromBottom(int(h*prob_to_pianoRoll_Ratio)));
        }
    };

    /** A child component of UI::XYPad explicitely used in drum piano rolls.
     * The pad can automaticly update the horizontal threshold lines in
     * PianoRoll_InteractiveIndividualBlockWithProbability widgets for a given voice
     *
     */
    class XYPadWithtListeners : public UI::XYPad
    {
    public:
        vector<InteractiveIndividualBlockWithProbability*> ListenerWidgets;
        LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* max_num_to_modelThread_que;
        LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* sample_thresh_to_modelThread_que;

        /**
         *
         * @param x_min_ min number of hits allowed
         * @param x_max_ max number of hits allowed
         * @param x_default_ default number of hits allowed
         * @param y_min_ lowest sampling thresh
         * @param y_max_ highest sampling thresh
         * @param y_default_ default sampling thresh
         * @param max_num_to_modelThread_quePntr (LockFreeQueue<float, GeneralSettings::gui_io_queue_size>) queue to send x value to  ModelThread for updating sampling conditions from model
         * @param sample_thresh_to_modelThread_quePntr (LockFreeQueue<float, GeneralSettings::gui_io_queue_size>) queue to send y value to  ModelThread for updating sampling conditions from model
         */
        XYPadWithtListeners(float x_min_, float x_max_, float x_default_, float y_min_, float y_max_, float y_default_,
                              LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* max_num_to_modelThread_quePntr,
                              LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* sample_thresh_to_modelThread_quePntr):
            XYPad(x_min_, x_max_, x_default_, y_min_, y_max_, y_default)
        {
            max_num_to_modelThread_que = max_num_to_modelThread_quePntr;
            sample_thresh_to_modelThread_que =  sample_thresh_to_modelThread_quePntr;
        }

        /**
         * Adds PianoRoll_InteractiveIndividualBlockWithProbability instances to an internal vector.
         * THis way sampling thresholds in the "listener" widgets can be automatically updated
         * @param widget
         */
        void addWidget(InteractiveIndividualBlockWithProbability* widget)
        {
            ListenerWidgets.push_back(widget);
        }

        // double clicking hides/unhides the xy location)
        void mouseDoubleClick(const juce::MouseEvent& ev) override
        {
            XYPad::mouseDoubleClick(ev);
            BroadCastAllInfo();
        }

        // dragging moves the xy location
        void mouseDrag(const juce::MouseEvent& ev) override
        {
            XYPad::mouseDrag(ev);
            BroadCastAllInfo();
        }

        // sends sampling thresholds to listener widgets
        void BroadCastThresholds()
        {
            // at least one widget should have been added using
            // addWidget(). If no listeners, you should use the basic
            // UI::XYPad component
            assert (ListenerWidgets.size()>0);

            auto thresh = XYPad::getYValue();
            max_num_to_modelThread_que->push(getXValue());
            sample_thresh_to_modelThread_que->push(thresh);

            for (int i=0; i<ListenerWidgets.size(); i++)
            {
                ListenerWidgets[i]->probabilityCurveWidgetPntr->setSamplingThreshold(thresh);
            }
        }

        // sends all info to listeners
        void BroadCastAllInfo()
        {
            vector<float> probabilities {};
            vector<float> probs_for_widgets_idx {};

            int current_widget_idx = 0;
            for (auto & ListenerWidget : ListenerWidgets)
            {
                auto hit_prob_ = ListenerWidget->probabilityCurveWidgetPntr->hit_prob;
                if (hit_prob_ > 0)

                    if (getYValue() <= hit_prob_)
                    {
                        probabilities.push_back(hit_prob_);
                        probs_for_widgets_idx.push_back(current_widget_idx);
                    }
                current_widget_idx++;
            }

            BroadCastThresholds();
        }

        // moves the xy location without mouse interaction
        void ForceMoveToActualValues(float x_ParameterValue_, float y_ParameterValue_)
        {
            XYPad::moveUsingActualValues(x_ParameterValue_, y_ParameterValue_);

            BroadCastAllInfo();
        }

        // changes default values
        void updateDefaultValues(float default_x_, float default_y_)
        {
            XYPad::updateDefaultValues(default_x_, default_y_);
            ForceMoveToActualValues(default_x_, default_y_);
            BroadCastAllInfo();
        }
    };
}



/*

// it should be inline because we don't have a separate cpp file
// otherwise, linking errors
// https://forum.juce.com/t/drawing-curve-with-3-points-or-help-with-quadratic-calculations/22330
*/
/**
 *  creates a concave up or down curve from (x1, y1) to (x2, y2)
 *  Used in SingleStepPianoRollBlock::ProbabilityLevelWidget
 * @param concaveup (true for concave up and false for concave down)
 * @return
 *//*

inline array<float, 4> getPathParams(float x1, float y1, float x2, float y2, bool concaveup)
{
    float amplitude = 100;

    float mult;
    if (concaveup)
        mult = 1.0f;
    else
        mult = -1.0f;

    float a = atan2f(mult * (y2-y1), x2-x1) + juce::float_Pi/2.0;
    float midx = (x2-x1)/2 + cos(a)*amplitude;
    //float midy = (y2-y1)/2 + y1 - sin(a)*amplitude;
    float midy = (y2-y1)/2 + sin(a)*amplitude;
    return {midx, midy, x2, y2};
}*/
