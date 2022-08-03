//
// Created by Behzad Haki on 2022-02-08.
//
#pragma once

#include <iostream>
#include <string>

#include "PluginProcessor.h"
#include "settings.h"

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

    void start_Thread(LockFreeQueue<Note, settings::note_queue_size>& note_quePntr)
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
        LockFreeQueue<Note, settings::note_queue_size>* note_queP;
};



class TextMessageLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    TextMessageLoggerTextEditor(): LoggerTextEditorTemplate()
    {
        TextEditorLabel.setText ("TextMessage", juce::dontSendNotification);
        TextEditorLabel.attachToComponent (this, juce::Justification::top);
        TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        TextEditorLabel.setJustificationType (juce::Justification::top);
        addAndMakeVisible (TextEditorLabel);
    }

    void start_Thread(LockFreeQueue<string , settings::text_message_queue_size>& text_message_que)
    {
        this->text_message_queue = &text_message_que;
        this->startThread();
    }

    void QueueDataProcessor() override
    {
        while (text_message_queue->getNumReady() > 0)
        {
            string msg;
            text_message_queue->ReadFrom(&msg, 1); // here cnt result is 3

            juce::MessageManagerLock mmlock;
            if (msg == "clear" or msg == "Clear") {
                this->clear();
            }
            else
            {
                insertTextAtCaret(msg);
                insertTextAtCaret(juce::newLine);
            }
        }
    }

private:
    LockFreeQueue<string, settings::text_message_queue_size>* text_message_queue;
};


/*
void showMessageinEditor(LockFreeQueue<string, settings::text_message_queue_size>* text_message_queue,
                  string message, string header = "", bool clearFirst=false)
{
    auto txt = string("clear");
    if (clearFirst)
    {
        text_message_queue->WriteTo(&txt, 1);
    }
    if (header == "")
    {
        txt = string("---------------------");
        text_message_queue->WriteTo(&txt, 1);
    }
    else
    {
        txt = header;
        text_message_queue->WriteTo(&txt, 1);
    }

    txt = message;
    text_message_queue->WriteTo(&txt, 1);
}
*/
