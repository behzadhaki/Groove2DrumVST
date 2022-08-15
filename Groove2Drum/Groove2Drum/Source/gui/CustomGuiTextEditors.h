//
// Created by Behzad Haki on 2022-08-03.
//

#ifndef JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H
#define JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H


#include <iostream>
#include <string>

#include "../PluginProcessor.h"
#include "../settings.h"

/*
  *
  * A template class for A TextEditor that runs a separate thread in which continuosly
  * reads data (if any) from a provided queue and prints it on screen
  * to use the template (see example class MidiNoteValueLoggerTextEditor):
  *     1.  override the QueueDataProcessor() --> define what needs to be done
  *             on the data received from the queue
  *     2.  implement a startThreadUsingProvidedResources method which gets access to a queue and
  *             calls the run() method
  * TO RECAP! MUST override QueueDataProcessor and implement a startThreadUsingProvidedResources method
*/
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


/**
 * A GUI TextEditor with a separate thread for receiving and displaying
 * Note instances
 */
class BasicNoteStructLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    BasicNoteStructLoggerTextEditor();

    ~BasicNoteStructLoggerTextEditor() override;

    void startThreadUsingProvidedResources(LockFreeQueue<BasicNote, settings::gui_io_queue_size>* note_quePntr);

    void QueueDataProcessor() override;

private:
    LockFreeQueue<BasicNote, settings::gui_io_queue_size>* note_queP;

    int numNotesPrintedOnLine;

};


/**
 * A GUI TextEditor with a separate thread for receiving and displaying
 * String messages
 */
class TextMessageLoggerTextEditor: public LoggerTextEditorTemplate
{
public:
    TextMessageLoggerTextEditor();
    ~TextMessageLoggerTextEditor() override;

    void startThreadUsingProvidedResources(StringLockFreeQueue<settings::gui_io_queue_size>* text_message_quePntr);
    void QueueDataProcessor() override;

private:
    StringLockFreeQueue<settings::gui_io_queue_size>* text_message_queue;

};





#endif //JUCECMAKEREPO_CUSTOMGUITEXTEDITORS_H
