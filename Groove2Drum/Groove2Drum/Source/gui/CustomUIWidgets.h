//
// Created by Behzad Haki on 2022-08-20.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>


class XYPlane : public juce::Component
{
public:
    float x_min; float x_max; float x_default;
    float y_min; float y_max; float y_default;
    float x_ParameterValue; float y_ParameterValue; // actual values of x or y parameters


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

        auto x_ = max(min((x_ParameterValue - x_min) * w / (x_max - x_min), w), 0.0f);
        auto y_ = max(min((1 - (y_ParameterValue - y_min) / (y_max - y_min)) * h, h), 0.0f);

        g.drawLine(0.0f, y_, x_, y_);
        g.drawLine(x_, h, x_, y_);

    }

    void moveToCoordinate(juce::Point<float> xyCoordinate)
    {
        auto w = float(getWidth());
        auto h = float(getHeight());

        float x_coor = max(min(xyCoordinate.getX(), w), 0.0f);
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
        moveUsingActualValues(x_default, y_default);
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

