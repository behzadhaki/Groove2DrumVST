//
// Created by Behzad Haki on 2022-08-18.
//

#pragma once

#include "../PluginProcessor.h"
#include "CustomGuiTextEditors.h"
#include "InteractivePianoRollBlock.h"

using namespace std;

// ============================================================================================================
// ==========            The Tab used for debugging using CustomGuiTextEditor Instances           =============
// ============================================================================================================

class TextDebugTabWidget: public juce::Component
{
public:
    explicit TextDebugTabWidget(MidiFXProcessor& MidiFXProcessorPointer, int size_width, int size_height)
    {
        // Sample rate text
        SampleRateLabel.setText("THIS TAB IS FOR DEBUGGING USING TEXTEDITORS", juce::dontSendNotification);
        SampleRateLabel.setColour(SampleRateLabel.backgroundColourId, juce::Colour(220, 100, 60));
        SampleRateLabel.setBounds (0, 0, 400, 30);

        // Create TextEditor for BasicNote Struct
        basicNoteStructLoggerTextEditor = MidiFXProcessorPointer.basicNoteStructLoggerTextEditor.get();
        basicNoteStructLoggerTextEditor->setMultiLine (true);
        basicNoteStructLoggerTextEditor->setBounds (100, 40, 500, 100);
        addAndMakeVisible (basicNoteStructLoggerTextEditor);

        // Create TextEditor for Text Messages
        textMessageLoggerTextEditor = MidiFXProcessorPointer.textMessageLoggerTextEditor.get();
        textMessageLoggerTextEditor->setMultiLine (true);
        textMessageLoggerTextEditor->setBounds (100, 200, 500, 100);
        addAndMakeVisible (MidiFXProcessorPointer.textMessageLoggerTextEditor.get());

        // Create TextEditor for Text Messages
        textMessageLoggerTextEditor_mainprocessBlockOnly = MidiFXProcessorPointer.textMessageLoggerTextEditor_mainprocessBlockOnly.get();
        textMessageLoggerTextEditor_mainprocessBlockOnly->setMultiLine (true);
        textMessageLoggerTextEditor_mainprocessBlockOnly->setBounds (100, 400, 500, 100);
        addAndMakeVisible (MidiFXProcessorPointer.textMessageLoggerTextEditor_mainprocessBlockOnly.get());

        setSize(size_width, size_height);

    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {}

private:
    BasicNoteStructLoggerTextEditor* basicNoteStructLoggerTextEditor;
    TextMessageLoggerTextEditor* textMessageLoggerTextEditor;
    TextMessageLoggerTextEditor* textMessageLoggerTextEditor_mainprocessBlockOnly;
    juce::Label SampleRateLabel;
};

// ============================================================================================================
// ==========                       All Tabs are collected and placed here                        =============
// ==========                   See tutorial Juce/Examples/GUI/AccessibilityDemo                  =============
// ============================================================================================================

class MultiTabComponent: public juce::Component
{
public:
    explicit MultiTabComponent(MidiFXProcessor& MidiFXProcessorPointer, int size_width, int size_height)
    {
        // Instantiate the widgets here
        textDebugTabWidget = make_unique<TextDebugTabWidget>(MidiFXProcessorPointer, size_width, size_height);
        auto border_c = juce::Colours::blue;
        int grey_level = (int) 0.9f * 255;
        auto background_c = juce::Colour::fromRGBA(grey_level,grey_level,grey_level,1);

        testComponent1 = make_unique<InteractivePianoRollBlock>(true, border_c, background_c);
        testComponent2 = make_unique<ProbabilityLevelWidget>(border_c, background_c);
        testComponent3 = make_unique<InteractivePianoRollBlockWithProbability>(size_width, size_height, true, border_c, background_c);

        const auto tabColour = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker (0.1f);

        // add Tabs
        tabs.addTab ("Debugger", tabColour, textDebugTabWidget.get(), true);
        tabs.addTab ("Debugger 1", tabColour, testComponent1.get(), true);
        tabs.addTab ("Debugger 2", tabColour, testComponent2.get(), true);
        tabs.addTab ("Debugger 3", tabColour, testComponent3.get(), true);


        tabs.setBounds (0, 0, size_width, size_height);
        addAndMakeVisible (tabs);

        setSize(size_width, size_height);

        //
        testComponent1->addEvent(1, .2f, -.25);

        testComponent2->setProbability(0.5f);

        testComponent3->addEvent(1, .2f, 0.25, 0.5);

    }

    void paint(juce::Graphics&  g) override
    {
        g.fillAll(getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {}

private:
    juce::TooltipWindow tooltipWindow { nullptr, 100 };

    juce::TabbedComponent tabs { juce::TabbedButtonBar::Orientation::TabsAtTop };


    unique_ptr<TextDebugTabWidget> textDebugTabWidget;
    unique_ptr<InteractivePianoRollBlock> testComponent1;
    unique_ptr<ProbabilityLevelWidget> testComponent2;
    unique_ptr<InteractivePianoRollBlockWithProbability> testComponent3;
};

