#pragma once

#include "PluginProcessor.h"
#include "CustomLoggerTextEditors.h"
#include "queue62.hpp"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
    PlayheadLoggerTextEditor PlayheadLoggerTextEditor;
    NoteStructLoggerTextEditor NoteStructLoggerTextEditor;
    MidiMsgPlayHeadStructLoggerTextEditor MidiMsgPlayHeadStructLoggerTextEditor;

    juce::Label SampleRateLabel;
};

