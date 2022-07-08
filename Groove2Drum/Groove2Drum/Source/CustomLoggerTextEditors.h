//
// Created by Behzad Haki on 2022-02-08.
//
#pragma once

#include <iostream>
#include <string>

#include "PluginProcessor.h"


/*
  *
  * A template class for A TextEditor that runs a separate thread in which continuosly
  * reads data (if any) from a provided queue and prints it on screen
  * to use the template (see example class MidiNoteValueLoggerTextEditor):
  *     1.  override the QueueDataProcessor() --> define what needs to be done
  *             on the data received from the queue
  *     2.  implement a start_Thread method which gets access to a queue and
  *             calls the run() method
  * TO RECAP! MUST override QueueDataProcessor and implement a start_Thread method
*/
class LoggerTextEditorTemplate: public juce::TextEditor, public juce::Thread
{
public:
    LoggerTextEditorTemplate():juce::TextEditor(), juce::Thread("Thread"){
        this->setMultiLine (true);
        this->setReturnKeyStartsNewLine (true);
        this->setReadOnly (false);
        this->setScrollbarsShown (true);
        this->setCaretVisible (true);
    }
    ~LoggerTextEditorTemplate() override { stopThread(10) ;}

    // Override this with the task to be done on received data from queue
    // See examples of the child classes below
    virtual void QueueDataProcessor() {}

    // No need to override in the children classes
    void run() override
    {
        bool bExit = threadShouldExit();
        while (!bExit)
        {
            QueueDataProcessor();
            bExit = threadShouldExit();
            sleep (1); // avoid burning CPU, if reading is returning immediately
        }
    }

    juce::Label TextEditorLabel;

};

/*
     * A dedicated TextEditor which displays midi messages received from a queue
     * To use in PluginEditor:
     *     1.   Instantiate in the private section of the editor class in PluginEditor.h
     *
     *          private:
     *               MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
     *
     *     2.   Add the following lines to the constructor of the editor in PluginEditor.cpp
     *
     *          addAndMakeVisible (MidiNoteValueLoggerTextEditor);
     *          MidiNoteValueLoggerTextEditor.start_Thread(queue_pointer);
     *              NOTE:   queue_pointer comes from the processor.
     *                      eg. &MidiFXProcessorPointer.midi_message_que
     *
*/
class MidiNoteValueLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    MidiNoteValueLoggerTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("Midi Note:", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(spsc_queue<juce::MidiMessage, settings::midi_queue_size>* midi_message_que_P)
    {
        midi_message_que = midi_message_que_P;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        juce::MidiMessage message_to_read;

        while (midi_message_que->read_available() > 0)
        {
            midi_message_que->pop(message_to_read); // here cnt result is 3
            if (message_to_read.isNoteOn()){
                // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
                // read from midi_message_que
                juce::MessageManagerLock mmlock;
                insertTextAtCaret(
                    juce::String(
                        message_to_read.getNoteNumber())+juce::newLine);
            }
        }
    }

private:
    spsc_queue<juce::MidiMessage, settings::midi_queue_size>* midi_message_que;
};

/*
     * A dedicated TextEditor which displays playhead info from a queue
     * To use in PluginEditor:
     *     1.   Instantiate in the private section of the editor class in PluginEditor.h
     *
     *          private:
     *               MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
     *
     *     2.   Add the following lines to the constructor of the editor in PluginEditor.cpp
     *
     *          addAndMakeVisible (MidiNoteValueLoggerTextEditor);
     *          MidiNoteValueLoggerTextEditor.start_Thread(queue_pointer);
     *              NOTE:   queue_pointer comes from the processor.
     *                      eg. &MidiFXProcessorPointer.midi_message_que
     *
*/
class PlayheadLoggerTextEditor: public LoggerTextEditorTemplate
{

public:
    PlayheadLoggerTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("Playhead", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::green);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(
        spsc_queue<juce::AudioPlayHead::CurrentPositionInfo, settings::playhead_queue_size>* playhead_quePntr)
    {
        playhead_queP = playhead_quePntr;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        juce::AudioPlayHead::CurrentPositionInfo read_info;

        while (playhead_queP->read_available() > 0)
        {
            playhead_queP->pop(read_info); // here cnt result is 3
            if (read_info.isPlaying){
                // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
                // read from midi_message_que
                juce::MessageManagerLock mmlock;

                insertTextAtCaret(juce::String(read_info.ppqPosition)+juce::newLine);
                insertTextAtCaret(juce::String());
            }
        }
    }

private:
    spsc_queue<juce::AudioPlayHead::CurrentPositionInfo, settings::playhead_queue_size>* playhead_queP;
};

/*
     * A dedicated TextEditor which displays playhead info from a queue
     * To use in PluginEditor:
     *     1.   Instantiate in the private section of the editor class in PluginEditor.h
     *
     *          private:
     *               MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
     *
     *     2.   Add the following lines to the constructor of the editor in PluginEditor.cpp
     *
     *          addAndMakeVisible (MidiNoteValueLoggerTextEditor);
     *          MidiNoteValueLoggerTextEditor.start_Thread(queue_pointer);
     *              NOTE:   queue_pointer comes from the processor.
     *                      eg. &MidiFXProcessorPointer.midi_message_que
     *
*/
class NoteStructLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    NoteStructLoggerTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("Midi#/Vel/Actual Onset ppq", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(spsc_queue<Note, settings::note_queue_size>* note_quePntr)
    {
        note_queP = note_quePntr;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        while (note_queP->read_available() > 0)
        {
            Note note;
            note_queP->pop(note); // here cnt result is 3
            juce::MessageManagerLock mmlock;
            insertTextAtCaret(
                juce::String(note.note)+"\t "+
                juce::String(note.velocity)+"\t "+
                juce::String(note.time.ppq, 5));
            insertTextAtCaret(juce::newLine);

        }
    }

private:
    spsc_queue<Note, settings::note_queue_size>* note_queP;
};

/*
     * A dedicated TextEditor which displays playhead info from a queue
     * To use in PluginEditor:
     *     1.   Instantiate in the private section of the editor class in PluginEditor.h
     *
     *          private:
     *               MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
     *
     *     2.   Add the following lines to the constructor of the editor in PluginEditor.cpp
     *
     *          addAndMakeVisible (MidiNoteValueLoggerTextEditor);
     *          MidiNoteValueLoggerTextEditor.start_Thread(queue_pointer);
     *              NOTE:   queue_pointer comes from the processor.
     *                      eg. &MidiFXProcessorPointer.midi_message_que
     *
*/
class MidiMsgPlayHeadStructLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    MidiMsgPlayHeadStructLoggerTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("Midi Note &  All Playhead", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::red);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(spsc_queue<MidiMsgPlayHead, settings::midi_queue_size>* midiMsgPlayheadPntr)
    {
        midiMsgPlayhead_queP = midiMsgPlayheadPntr;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        while (midiMsgPlayhead_queP->read_available() > 0)
        {
            MidiMsgPlayHead midiMsgPlayHead;
            midiMsgPlayhead_queP->pop(midiMsgPlayHead); // here cnt result is 3
            juce::MessageManagerLock mmlock;

            if(midiMsgPlayHead.MidiMessage.isNoteOn()){
                std::string note = to_string(midiMsgPlayHead.MidiMessage.getNoteNumber());
                note.resize (5, ' ');
                insertTextAtCaret("Note: " + note + " | ");

                std::string Onset_samples = to_string(midiMsgPlayHead.MidiMessage.getTimeStamp());
                Onset_samples.resize (5, ' ');
                if (Onset_samples=="0.0  "){Onset_samples="  0  ";}
                insertTextAtCaret("Onset_samples: " + Onset_samples + " | ");

                std::string qpm = to_string(midiMsgPlayHead.playheadInfo.bpm);
                qpm.resize (5, ' ');
                insertTextAtCaret("Frame_qpm: " + qpm + " | ");

                std::string ppqPosition = to_string(midiMsgPlayHead.playheadInfo.ppqPosition);
                ppqPosition.resize (5, ' ');
                insertTextAtCaret("Frame_ppq: " + ppqPosition);
                insertTextAtCaret(juce::newLine);

            }

        }
    }

private:
    spsc_queue<MidiMsgPlayHead, settings::midi_queue_size>* midiMsgPlayhead_queP;

};

/*
     * A dedicated TextEditor which displays playhead info from a queue
     * To use in PluginEditor:
     *     1.   Instantiate in the private section of the editor class in PluginEditor.h
     *
     *          private:
     *               MidiNoteValueLoggerTextEditor MidiNoteValueLoggerTextEditor;
     *
     *     2.   Add the following lines to the constructor of the editor in PluginEditor.cpp
     *
     *          addAndMakeVisible (MidiNoteValueLoggerTextEditor);
     *          MidiNoteValueLoggerTextEditor.start_Thread(queue_pointer);
     *              NOTE:   queue_pointer comes from the processor.
     *                      eg. &MidiFXProcessorPointer.midi_message_que
     *
*/
class TorchTensorTextEditor: public LoggerTextEditorTemplate
{
public:
    TorchTensorTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("Torch Tensor", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::red);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(spsc_queue<torch::Tensor, settings::torch_tensor_queue_size>* torch_tensor_quePointer)
    {
        torch_tensor_queP = torch_tensor_quePointer;

        //torch_tensor_queP ->push(torch::rand({2, 3})); // TODO added for testing --> to be removed

        this->startThread();
    }

    void QueueDataProcessor() override
    {
        while (torch_tensor_queP->read_available() > 0){
            torch::Tensor tensor_read;
            torch_tensor_queP->pop(tensor_read);

            std::ostringstream stream;
            stream << tensor_read;

            juce::MessageManagerLock mmlock;
            juce::TextEditor::insertTextAtCaret((juce::String(stream.str())));
            juce::TextEditor::insertTextAtCaret(juce::NewLine());
        }
        /*while (midiMsgPlayhead_queP->read_available() > 0)
        {
            MidiMsgPlayHead midiMsgPlayHead;
            midiMsgPlayhead_queP->pop(midiMsgPlayHead); // here cnt result is 3
            juce::MessageManagerLock mmlock;

            if(midiMsgPlayHead.MidiMessage.isNoteOn()){
                std::string note = to_string(midiMsgPlayHead.MidiMessage.getNoteNumber());
                note.resize (5, ' ');
                insertTextAtCaret("Note: " + note + " | ");

                std::string Onset_samples = to_string(midiMsgPlayHead.MidiMessage.getTimeStamp());
                Onset_samples.resize (5, ' ');
                if (Onset_samples=="0.0  "){Onset_samples="  0  ";}
                insertTextAtCaret("Onset_samples: " + Onset_samples + " | ");

                std::string qpm = to_string(midiMsgPlayHead.playheadInfo.bpm);
                qpm.resize (5, ' ');
                insertTextAtCaret("Frame_qpm: " + qpm + " | ");

                std::string ppqPosition = to_string(midiMsgPlayHead.playheadInfo.ppqPosition);
                ppqPosition.resize (5, ' ');
                insertTextAtCaret("Frame_ppq: " + ppqPosition);
                insertTextAtCaret(juce::newLine);

            }

        }*/
    }

private:
    spsc_queue<torch::Tensor, settings::torch_tensor_queue_size>* torch_tensor_queP;
};

