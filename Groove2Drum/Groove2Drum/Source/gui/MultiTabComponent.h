//
// Created by Behzad Haki on 2022-08-18.
//

#pragma once

#include "../PluginProcessor.h"
#include "CustomGuiTextEditors.h"
#include "PianoRoll_InteractiveIndividualBlock.h"
#include "PianoRoll_InteractiveMonotonicGroove.h"
#include "PianoRoll_GeneratedDrums.h"

using namespace std;

// ============================================================================================================
// ==========            The Tab used for debugging using CustomGuiTextEditor Instances           =============
// ============================================================================================================

class TextDebugTabWidget: public juce::Component
{
public:
    explicit TextDebugTabWidget(MidiFXProcessor& MidiFXProcessorPointer/*, int size_width, int size_height*/)
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

        //setSize(size_width, size_height);

    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {

        auto area = getLocalBounds();
        setBounds(area);
        /*auto area = getLocalBounds();

        auto h = float(getHeight());
        basicNoteStructLoggerTextEditor->setBounds(area.removeFromTop(int(h * .2)));
        textMessageLoggerTextEditor->setBounds(area.removeFromTop(int(h * .4)));
        textMessageLoggerTextEditor_mainprocessBlockOnly->setBounds(area.removeFromTop(int(h * .4)));*/
    }

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
    juce::TooltipWindow tooltipWindow { nullptr, 100 };

    juce::TabbedComponent tabs { juce::TabbedButtonBar::Orientation::TabsAtTop };

    unique_ptr<TextDebugTabWidget> textDebugTabWidget;
    unique_ptr<PianoRoll_InteractiveMonotonicGroove> monotonicGroovePianoRoll;
    unique_ptr<MonotonicGrooveWidget> monotonicGrooveWidget;
    unique_ptr<PianoRoll_GeneratedDrums_AllVoices> DrumsPianoRoll;

    explicit MultiTabComponent(MidiFXProcessor& MidiFXProcessorPointer/*, int size_width, int size_height*/, int time_steps_, float step_resolution_ppq, int num_voices_)
    {
        // Instantiate the widgets here
        textDebugTabWidget = make_unique<TextDebugTabWidget>(MidiFXProcessorPointer/*, size_width, size_height*/);

        auto grey_level = juce::uint8(0.9f * 255);
        auto background_c = juce::Colour::fromRGBA(grey_level,grey_level,grey_level,1);

        monotonicGroovePianoRoll = make_unique<PianoRoll_InteractiveMonotonicGroove>(true, time_steps_, step_resolution_ppq,4, 4, "TEST INTERACTIVE GROOVE");
        monotonicGrooveWidget = make_unique<MonotonicGrooveWidget>( time_steps_, step_resolution_ppq,4, 4);
        DrumsPianoRoll = make_unique<PianoRoll_GeneratedDrums_AllVoices>(time_steps_, step_resolution_ppq, 4, 4, nine_voice_kit_labels, nine_voice_kit);

        const auto tabColour = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker (0.1f);

        // add Tabs
        tabs.addTab ("Full Drums", tabColour, DrumsPianoRoll.get(), true);
        //tabs.addTab ("Debugger", tabColour, textDebugTabWidget.get(), true);
        tabs.addTab ("Single monotonicGroovePianoRoll", tabColour, monotonicGroovePianoRoll.get(), true);
        tabs.addTab ("Full Monotonic", tabColour, monotonicGrooveWidget.get(), true);


        addAndMakeVisible (tabs);


    }

    void paint(juce::Graphics&  g) override
    {
        g.fillAll(getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {
        auto area = getLocalBounds();
        textDebugTabWidget->setBounds(area);
        monotonicGroovePianoRoll->setBounds(area);
        monotonicGrooveWidget->setBounds(area);
        DrumsPianoRoll->setBounds(area);

        tabs.setBounds (area);


        //setSize(size_width, size_height);

    }


};

