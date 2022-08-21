//
// Created by Behzad Haki on 2022-08-18.
//
#pragma once

#include "CustomUIWidgets.h"

// ============================================================================================================
// ==========                       The smallest component in each piano roll                     =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ============================================================================================================

class PianoRoll_InteractiveIndividualBlock : public juce::Component
{
public:
    PianoRoll_InteractiveIndividualBlock(bool isClickable_, juce::Colour backgroundcolor_, int grid_index_, int voice_num_ = 0) {
        grid_index = grid_index_;
        backgroundcolor = backgroundcolor_;
        isClickable = isClickable_;
        hit = 0;
        velocity = 0;
        location = 0;
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
            g.setColour(juce::Colour::fromFloatRGBA(1.0f,0.7f,0.0f, 1.0f));

            auto x_pos = location * w;
            auto y_pos = (float)(1 - velocity) * h;
            juce::Point<float> p1 {x_pos, h};
            juce::Point<float> p2 {x_pos, y_pos};
            auto thickness = 3.0f;
            g.drawLine ({p1, p2}, thickness);

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
            hit = 1;
            location = ev.position.getX() / (float) getWidth();
            location = min(max(0.0f, location), .99f);
            velocity = 1 - ev.position.getY()/ (float)getHeight() ;
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& ev)
    {
        if (isClickable)
        {
            hit = 1;
            location = ev.position.getX() / (float) getWidth();
            location = min(max(0.0f, location), .99f);
            velocity = 1 - ev.position.getY()/ (float)getHeight() ;
            repaint();
        }
    }
    void mouseDoubleClick(const juce::MouseEvent& ) override
    {
        if (isClickable)
        {
            hit = 0;
            location = 0;
            velocity = 0;
            repaint();
            sendDataToQueue();
        }
    }

    void addEvent(int hit_, float velocity_, float location_)
    {
        assert (hit == 0 or hit == 1);

        hit = hit_;
        velocity = velocity_;
        location = location_;
        repaint();
        sendDataToQueue();
    }

    void addEventWithPPQ(int hit_, float velocity_, float ppq_, float step_resolution)
    {
        location = fmod(ppq_, step_resolution)/step_resolution;
        addEvent(hit_, velocity_, location);
    }

    void sendDataToQueue()
    {
        // todo to be implemented
    }

    int getHit() const
    {
        return hit;
    }
    float getVelocity() const
    {
        return velocity;
    }
    float getLocation() const
    {
        return location;
    }
    float getLocationPPQ(float step_resolution_ppq)
    {
        return (grid_index+location)*step_resolution_ppq;
    }
    int getGrid_index() const
    {
        return grid_index;
    }
    int getVoiceNumber()
    {
        return voice_num;
    }
    /*void resized() override {
        auto area = getLocalBounds();
        this->setBounds(area);
    }*/
private:
    bool isClickable;
    int hit;
    float velocity;
    float location;
    juce::Colour backgroundcolor;
    int grid_index; // time step corresponding to the block
    int voice_num;
};


class ProbabilityLevelWidget: public juce::Component
{
public:
    juce::Colour backgroundcolor;
    float line_thickness;
    float hit_prob = 0;
    float sampling_threshold = 0;

    ProbabilityLevelWidget(juce:: Colour backgroundcolor_, float line_thickness_=2)
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

        if (hit_prob > 0)
        {
            juce::Point<float> corner1 {w * .45f, h};
            juce::Point<float> corner2 {w * 0.55f, (1.0f-hit_prob)*h};
            g.setColour(juce::Colours::red);

            juce::Rectangle<float> area (corner1, corner2);
            g.setColour (juce::Colours::darkkhaki);
            g.fillRect (area);
        }
    }

    void setProbability(float hit_prob_)
    {
        hit_prob = hit_prob_;
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


        probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(juce::Colours::black /*backgroundcolor_*/);
        addAndMakeVisible(probabilityCurveWidgetPntr.get());
    }

    void addEvent(int hit_, float velocity_, float location, float hit_prob_)
    {
        pianoRollBlockWidgetPntr->addEvent(hit_, velocity_, location);
        probabilityCurveWidgetPntr->setProbability(hit_prob_);
    }

    void addEventWithPPQ(int hit_, float velocity_, float ppq_, float hit_prob_, float step_resolution)
    {
        pianoRollBlockWidgetPntr->addEventWithPPQ(hit_, velocity_, ppq_, step_resolution);
        probabilityCurveWidgetPntr->setProbability(hit_prob_);
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
