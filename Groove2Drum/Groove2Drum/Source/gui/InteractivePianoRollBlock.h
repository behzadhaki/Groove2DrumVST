//
// Created by Behzad Haki on 2022-08-18.
//
#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

// ============================================================================================================
// ==========                       The smallest component in each piano roll                     =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ============================================================================================================


class InteractivePianoRollBlock : public juce::Component
{
public:
    juce::Colour bordercolor;
    juce::Colour backgroundcolor;

    InteractivePianoRollBlock(bool isClickable_, juce::Colour bordercolor_, juce::Colour backgroundcolor_) {
        bordercolor = bordercolor_;
        backgroundcolor = backgroundcolor_;
        isClickable = isClickable_;
        hit = 0;
        velocity = 0;
        offset = 0;
    }

    void paint(juce::Graphics& g) override
    {
        //background colour
        g.fillAll(backgroundcolor);

        // draw border
        g.setColour(bordercolor);
        g.drawRoundedRectangle (getLocalBounds().toFloat(), 5.0f, 3.0f);

        if (hit == 1)
        {
            // draw line
            g.setColour(juce::Colours::red);
            auto x_pos = (float)(offset + 0.5) * getWidth();
            auto y_pos = (float)(1 - velocity) * getHeight();
            juce::Point<float> p1 {x_pos, (float)getHeight()};
            juce::Point<float> p2 {x_pos, y_pos};
            auto thickness = 2.0f;
            g.drawLine ({p1, p2}, thickness);

            // g.drawLine(x_pos, getHeight(),x_pos, y_pos);
        }

    }

    void mouseDown(const juce::MouseEvent& ev) override
    {
        if (isClickable)
        {
            hit = 1;
            offset = -0.5 + ev.position.getX() / getWidth();
            velocity = 1 - ev.position.getY()/ getHeight() ;
            repaint();
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& ev) override
    {
        if (isClickable)
        {
            hit = 0;
            offset = 0;
            velocity = 0;
            repaint();
        }
    }

    void addEvent(int hit_, float velocity_, float offset_)
    {
        assert (hit == 0 or hit == 1);

        hit = hit_;
        velocity = velocity_;
        offset = offset_;
        repaint();
    }

    int getHit() const
    {
        return hit;
    }
    float getVelocity() const
    {
        return velocity;
    }
    float getOffset() const
    {
        return offset;
    }

private:
    bool isClickable;
    int hit;
    float velocity;
    float offset;



};


class ProbabilityLevelWidget: public juce::Component
{
public:
    juce::Colour bordercolor;
    juce::Colour backgroundcolor;
    float line_thickness;
    float hit_prob = 0;

    ProbabilityLevelWidget(juce::Colour bordercolor_, juce:: Colour backgroundcolor_, float line_thickness_=2)
    {
        bordercolor = bordercolor_;
        backgroundcolor = backgroundcolor_;
        line_thickness = line_thickness_;
    }

    void paint(juce::Graphics& g) override
    {
        //background colour
        g.fillAll(backgroundcolor);

        // draw border
        g.setColour(bordercolor);
        g.drawRoundedRectangle (getLocalBounds().toFloat(), 5.0f, 3.0f);

        if (hit_prob > 0)
        {
            auto h = (float) getHeight();
            auto w = (float) getWidth();

            juce::Point<float> corner1 {w * 0.45f, h};
            juce::Point<float> corner2 {w * 0.55f, (1.0f-hit_prob)*h};

            g.setColour(juce::Colours::red);
            g.drawRect({corner1, corner2}, line_thickness);

        }

    }

    void setProbability(float hit_prob_)
    {
        hit_prob = hit_prob_;
        repaint();
    }


};


class InteractivePianoRollBlockWithProbability: public juce::Component
{
public:
    unique_ptr<InteractivePianoRollBlock> pianoRollBlockWidgetPntr;  // unique_ptr so as to allow for initialization in the constructor
    unique_ptr<ProbabilityLevelWidget> probabilityCurveWidgetPntr;         // component instance within which we'll draw the probability curve

    InteractivePianoRollBlockWithProbability(int size_width, int size_height, bool isClickable_, juce::Colour bordercolor_, juce::Colour backgroundcolor_)
    {
        pianoRollBlockWidgetPntr = make_unique<InteractivePianoRollBlock>(isClickable_, bordercolor_, backgroundcolor_);
        pianoRollBlockWidgetPntr->setBounds (0, 0, size_width, size_height*.75f);
        addAndMakeVisible(pianoRollBlockWidgetPntr.get());

        probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(bordercolor_, backgroundcolor_);
        probabilityCurveWidgetPntr->setBounds (0, size_height*.75f, size_width,  size_height*.25f);
        addAndMakeVisible(probabilityCurveWidgetPntr.get());

        setSize(size_width, size_height);

    }

    /*void paint(juce::Graphics& g) override
    {
        g.fillAll();
    }*/

    void addEvent(int hit_, float velocity_, float offset_, float hit_prob_)
    {
        pianoRollBlockWidgetPntr->addEvent(hit_, velocity_, offset_);
        probabilityCurveWidgetPntr->setProbability(hit_prob_);
        //repaint();
    }

    void resized() override {}


};