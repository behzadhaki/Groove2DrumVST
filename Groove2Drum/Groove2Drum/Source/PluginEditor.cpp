#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"


#include <torch/script.h> // One-stop header.

#include <iostream>

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    addAndMakeVisible(SampleRateLabel);

    SampleRateLabel.setText("MAKE SURE SAMPLE RATE IS "+juce::String(settings::sample_rate), juce::dontSendNotification);
    SampleRateLabel.setBounds (0, 0, 300, 30);

    /*// Create Message Box for Displaying Midi Notes, then start thread for reading
    // from midi_message_que queue and displaying in message box
    addAndMakeVisible (MidiNoteValueLoggerTextEditor);
    MidiNoteValueLoggerTextEditor.setBounds (100, 40, 100, 100);
    MidiNoteValueLoggerTextEditor.start_Thread(&MidiFXProcessorPointer.midi_message_que);

    // Create TextEditor for playhead info
    addAndMakeVisible (PlayheadLoggerTextEditor);
    PlayheadLoggerTextEditor.setMultiLine (true);
    PlayheadLoggerTextEditor.setBounds (300, 40, 100, 100);
    PlayheadLoggerTextEditor.start_Thread(&MidiFXProcessorPointer.playhead_que);*/


    // Create TextEditor for Note Struct
    addAndMakeVisible (NoteStructLoggerTextEditor);
    NoteStructLoggerTextEditor.setMultiLine (true);
    NoteStructLoggerTextEditor.setBounds (100, 40, 500, 100);
    NoteStructLoggerTextEditor.start_Thread(&MidiFXProcessorPointer.note_que);


    // Create TextEditor for midiMsgPlayhead Struct
    addAndMakeVisible (MidiMsgPlayHeadStructLoggerTextEditor);
    MidiMsgPlayHeadStructLoggerTextEditor.setMultiLine (true);
    MidiMsgPlayHeadStructLoggerTextEditor.setBounds (100, 180, 500, 100);
    MidiMsgPlayHeadStructLoggerTextEditor.start_Thread(&MidiFXProcessorPointer.midiMsgPlayhead_que);

    // Create TextEditor for displaying torch_tensors midiMsgPlayhead Struct


    addAndMakeVisible (TorchTensorTextEditor);
    TorchTensorTextEditor.setMultiLine (true);
    TorchTensorTextEditor.setBounds (100, 320, 500, 100);
    TorchTensorTextEditor.start_Thread(&MidiFXProcessorPointer.torchTensor_que);

    // Set window size
    setSize (620, 500);

    torch::jit::script::Module model;
    model = torch::jit::load("/Users/behzadhaki/Github/Groove2DrumVST/Groove2Drum/Groove2Drum/TorchScriptModels/misunderstood_bush_246-epoch_26_tst.pt");
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(torch::rand({32, 27})); // todo A wrapper needs to be implemented so as to predict h, v, o and stack them on top into a single hvo tensor within the wrappers internal forward
    auto outputs = model.forward(inputs);
    auto logits_v_o_tuples = outputs.toTuple();    // WE NEED
    auto hits = logits_v_o_tuples->elements()[0].toTensor();
    auto velocities = logits_v_o_tuples->elements()[1].toTensor();
    auto offset = logits_v_o_tuples->elements()[2].toTensor();

    // std::cout << res <<endl;

    MidiFXProcessorPointer.torchTensor_que.push(hits);
    MidiFXProcessorPointer.torchTensor_que.push(velocities);
    MidiFXProcessorPointer.torchTensor_que.push(offset);

    // model = torch::jit::load("/Users/behzadhaki/Library/Application Support/JetBrains/PyCharm2020.2/scratches/scriptmodule.pt");

}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


