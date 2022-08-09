//
// Created by Behzad Haki on 2022-08-03.
//

#ifndef JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H
#define JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H


#include <iostream>
#include <string>

#include "../PluginProcessor.h"
#include "../settings.h"

class LoggerTextEditorTemplate: public juce::TextEditor, public juce::Thread
{
public:
    LoggerTextEditorTemplate();

    void prepareToStop();
    // Override this with the task to be done on received data from queue
    // See examples of the child classes below
    virtual void QueueDataProcessor();

    // No need to override in the children classes
    void run() override;

    juce::Label TextEditorLabel;
};


class NoteStructLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    NoteStructLoggerTextEditor();

    ~NoteStructLoggerTextEditor() override;

    void start_Thread(LockFreeQueue<Note, settings::note_queue_size>* note_quePntr);

    void QueueDataProcessor() override;

private:
    LockFreeQueue<Note, settings::note_queue_size>* note_queP;

    int numNotesPrintedOnLine;

};


class TextMessageLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    TextMessageLoggerTextEditor();
    ~TextMessageLoggerTextEditor() override;

    void start_Thread(StringLockFreeQueue<settings::text_message_queue_size>* text_message_quePntr);
    void QueueDataProcessor() override;

private:
    StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue;

};


/*
void showMessageinEditor(LockFreeQueue<string, settings::text_message_queue_size>* text_message_queue,
                  string message, string header = "", bool clearFirst=false);
*/



#endif //JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H
