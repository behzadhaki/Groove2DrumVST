//
// Created by Behzad Haki on 2022-08-20.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../InterThreadFifos.h"

namespace UI
{
    class XYPlane : public juce::Component
{
public:
    float x_min; float x_max; float x_default;
    float y_min; float y_max; float y_default;
    float x_ParameterValue; float y_ParameterValue; // actual values of x or y parameters within the min/max ranges specified (NOT in terms of pixels!)


    XYPlane(float x_min_, float x_max_, float x_default_, float y_min_, float y_max_, float y_default_):
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

    void mouseDown(const juce::MouseEvent& ev) override
    {
        m_mousepos = ev.position;
        moveToCoordinate(m_mousepos);
    }

    void mouseDoubleClick(const juce::MouseEvent& ev) override
    {
        // more than 2 clicks goes back to default
        if (ev.getNumberOfClicks()>2)
        {
            moveUsingActualValues(x_default, y_default);
        }
        else
        {
            // shift with double click sets area to max
            if (ev.mods.isShiftDown())
            {
                moveToCoordinate(juce::Point<float> {(float)getX(), (float)getY()});
            }
            else // regular double click sets area to 0 --> no possible hits
            {
                moveToCoordinate(juce::Point<float> {0.0f, 0.0f});
            }
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
// ==========                       The smallest component in each piano roll                     =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ============================================================================================================


// it should be inline because we don't have a separate cpp file
// otherwise, linking errors
// https://forum.juce.com/t/drawing-curve-with-3-points-or-help-with-quadratic-calculations/22330
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
}

namespace SingleStepPianoRollBlock
{
    class PianoRoll_InteractiveIndividualBlock : public juce::Component
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

        PianoRoll_InteractiveIndividualBlock(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_, int voice_num_ = 0) {
            grid_index = grid_index_;
            backgroundcolor = backgroundcolor_;
            isClickable = isClickable_;
            hit = 0;
            velocity = 1;
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
                // draw a mustart colored line for the note hit
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

        void mouseUp(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                sendDataToQueue();
            }
        }

        void mouseDown(const juce::MouseEvent& ev)
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

        void mouseDoubleClick(const juce::MouseEvent& ) override
        {
            if (isClickable)
            {
                if (hit == 0)
                {
                    hit = 1;
                    velocity = 0.7f;
                    offset = 0.0f;
                }
                else
                {
                    hit = 0;
                    velocity = 0.0;
                    offset = 0.0f;
                }
                repaint();
                sendDataToQueue();
            }
        }

        void addEvent(int hit_, float velocity_, float offset_)
        {
            assert (hit == 0 or hit == 1);

            hit = hit_;
            velocity = velocity_;
            offset = offset_;

            repaint();
            sendDataToQueue();
        }

        void sendDataToQueue()
        {
            // todo to be implemented
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


    class ProbabilityLevelWidget: public juce::Component
    {
    public:
        juce::Colour backgroundcolor;
        float line_thickness;
        float hit_prob = 0;
        float sampling_threshold = 0;
        int hit;

        ProbabilityLevelWidget(juce:: Colour backgroundcolor_, int voice_number_, float line_thickness_=2)
        {
            backgroundcolor = backgroundcolor_;
            line_thickness = line_thickness_;
        }

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
            float control_rect_ratio = (1.0f - from_edge) * 0.5f; // ratio wise
            auto p = (float) proportionOfHeight(1.0f - hit_prob); // location of peak (ie probability)
            auto control_rect = juce::Rectangle<float> (juce::Point<float> ((float) proportionOfWidth(from_edge), h), juce::Point<float> ((float)proportionOfWidth(1.0f - from_edge), p));

            auto half_P = (float) proportionOfHeight(1.0f - hit_prob/2.0f);
            // g.setColour (juce::Colours::darkkhaki);
            g.setColour(prob_color_non_hit);
            myPath1.startNewSubPath (0.0f, h);

            myPath1.lineTo(juce::Point<float> (w, h));
            g.strokePath (myPath1, juce::PathStrokeType (2.0f));


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

        /*void setProbability(float hit_prob_)
        {
            if (hit_prob-hit_prob_>0.01 )
            {
                hit_prob = hit_prob_;
                repaint();
            }
        }*/

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


    class PianoRoll_InteractiveIndividualBlockWithProbability : public juce::Component
    {

    public:
        unique_ptr<PianoRoll_InteractiveIndividualBlock> pianoRollBlockWidgetPntr;  // unique_ptr so as to allow for initialization in the constructor
        unique_ptr<ProbabilityLevelWidget> probabilityCurveWidgetPntr;         // component instance within which we'll draw the probability curve


        PianoRoll_InteractiveIndividualBlockWithProbability(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_, int voice_num_)
        {
            pianoRollBlockWidgetPntr = make_unique<PianoRoll_InteractiveIndividualBlock>(isClickable_, backgroundcolor_, grid_index_, voice_num_);
            addAndMakeVisible(pianoRollBlockWidgetPntr.get());


            probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(juce::Colours::black /*backgroundcolor_*/, voice_num_);
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


    template<typename Iterator>
    inline std::vector<size_t> n_largest_indices(Iterator it, Iterator end, size_t n) {
        struct Element {
            Iterator it;
            size_t index;
        };

        std::vector<Element> top_elements;
        top_elements.reserve(n + 1);

        for(size_t index = 0; it != end; ++index, ++it) {
            top_elements.insert(std::upper_bound(top_elements.begin(), top_elements.end(), *it, [](auto value, auto element){return value > *element.it;}), {it, index});
            if (index >= n)
                top_elements.pop_back();
        }

        std::vector<size_t> result;
        result.reserve(top_elements.size());

        for(auto &element: top_elements)
            result.push_back(element.index);

        return result;
    }


    class XYPlaneWithtListeners : public UI::XYPlane
    {
    public:
        vector<PianoRoll_InteractiveIndividualBlockWithProbability*> ListenerWidgets;
        LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* max_num_to_modelThread_que;
        LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* sample_thresh_to_modelThread_que;

        XYPlaneWithtListeners(float x_min_, float x_max_, float x_default_, float y_min_, float y_max_, float y_default_,
                              LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* max_num_to_modelThread_quePntr,
                              LockFreeQueue<float, GeneralSettings::gui_io_queue_size>* sample_thresh_to_modelThread_quePntr):
            XYPlane(x_min_, x_max_, x_default_, y_min_, y_max_, y_default)
        {
            max_num_to_modelThread_que = max_num_to_modelThread_quePntr;
            sample_thresh_to_modelThread_que =  sample_thresh_to_modelThread_quePntr;
        }

        void addWidget(PianoRoll_InteractiveIndividualBlockWithProbability* widget)
        {
            ListenerWidgets.push_back(widget);
        }

        void mouseDown(const juce::MouseEvent& ev) override
        {
            XYPlane::mouseDown(ev);
            BroadCastAllInfo();
        }

        void mouseDoubleClick(const juce::MouseEvent& ev) override
        {
            XYPlane::mouseDoubleClick(ev);
            BroadCastAllInfo();
        }

        void mouseDrag(const juce::MouseEvent& ev) override
        {
            XYPlane::mouseDrag(ev);
            BroadCastAllInfo();
        }

        void BroadCastThresholds()
        {
            auto thresh = XYPlane::getYValue();
            max_num_to_modelThread_que->push(getXValue());
            sample_thresh_to_modelThread_que->push(thresh);

            for (int i=0; i<ListenerWidgets.size(); i++)
            {
                ListenerWidgets[i]->probabilityCurveWidgetPntr->setSamplingThreshold(thresh);
            }
        }

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

        void ForceMoveToActualValues(float x_ParameterValue_, float y_ParameterValue_)
        {
            XYPlane::moveUsingActualValues(x_ParameterValue_, y_ParameterValue_);

            BroadCastAllInfo();
        }

        void updateDefaultValues(float default_x_, float default_y_)
        {
            XYPlane::updateDefaultValues(default_x_, default_y_);
            ForceMoveToActualValues(default_x_, default_y_);
            BroadCastAllInfo();
        }
    };
}
