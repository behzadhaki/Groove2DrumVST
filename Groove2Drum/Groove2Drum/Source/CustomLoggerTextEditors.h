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

    void start_Thread(NoteQueue& note_quePntr)
    {
        note_queP = &note_quePntr;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        while (note_queP->getNumReady() > 0)
        {
            Note note;
            note_queP->ReadFrom(&note, 1); // here cnt result is 3
            juce::MessageManagerLock mmlock;
            insertTextAtCaret(
                juce::String(note.note)+"\t "+
                juce::String(note.velocity)+"\t "+
                juce::String(note.time.ppq, 5));
            insertTextAtCaret(juce::newLine);
        }
    }

    private:
    juce::ScopedPointer<NoteQueue> note_queP;
};


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
    }

private:
    spsc_queue<torch::Tensor, settings::torch_tensor_queue_size>* torch_tensor_queP;
};