//
// Created by Behzad Haki on 2022-08-03.
//

#include "CustomGuiTextEditors.h"


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
LoggerTextEditorTemplate::LoggerTextEditorTemplate():juce::TextEditor(), juce::Thread("Thread")
{
    this->setMultiLine (true);
    this->setReturnKeyStartsNewLine (true);
    this->setReadOnly (false);
    this->setScrollbarsShown (true);
    this->setCaretVisible (true);

}

void LoggerTextEditorTemplate::prepareToStop() { stopThread(10) ;}

// Override this with the task to be done on received data from queue
// See examples of the child classes below

void LoggerTextEditorTemplate::QueueDataProcessor() {}

// No need to override in the children classes
void LoggerTextEditorTemplate::run()
{
    bool bExit = threadShouldExit();
    while (!bExit)
    {
        QueueDataProcessor();
        bExit = threadShouldExit();
        sleep (1); // avoid burning CPU, if reading is returning immediately
    }
}



NoteStructLoggerTextEditor::NoteStructLoggerTextEditor(): LoggerTextEditorTemplate()
{

    TextEditorLabel.setText ("Midi#/Vel/Actual Onset ppq", juce::dontSendNotification);
    TextEditorLabel.attachToComponent (this, juce::Justification::top);
    TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    TextEditorLabel.setJustificationType (juce::Justification::top);
    addAndMakeVisible (TextEditorLabel);

    this->setCurrentThreadName("NoteStructureLoggerThread");

    numNotesPrintedOnLine = 0;
}

NoteStructLoggerTextEditor::~NoteStructLoggerTextEditor()
{
    this->prepareToStop();

}

void NoteStructLoggerTextEditor::start_Thread(LockFreeQueue<Note, settings::note_queue_size>* note_quePntr)
{
    note_queP = note_quePntr;
    this->startThread();
}

void NoteStructLoggerTextEditor::QueueDataProcessor()
{
    if (note_queP != nullptr)
    {
        while (note_queP->getNumReady() > 0 and not this->threadShouldExit())
        {
            Note note;
            note_queP->ReadFrom(&note, 1); // here cnt result is 3
            juce::MessageManagerLock mmlock;

            if(this->getTotalNumChars()>gui_settings::NoteStructLoggerTextEditor::maxChars)
            {
                this->clear();
                this->numNotesPrintedOnLine = 0;
            }

            insertTextAtCaret(
                "N: "+
                juce::String(note.note)+"\t "+
                juce::String(note.velocity, 2)+"\t "+
                juce::String(note.time.ppq, 4) + "||");

            this->numNotesPrintedOnLine += 1;

            if(this->numNotesPrintedOnLine > 0 and
                this->numNotesPrintedOnLine % gui_settings::NoteStructLoggerTextEditor::
                            nNotesPerLine == 0)
                insertTextAtCaret(juce::newLine);

        }
    }

}



TextMessageLoggerTextEditor::TextMessageLoggerTextEditor(): LoggerTextEditorTemplate()
{
    TextEditorLabel.setText ("TextMessage", juce::dontSendNotification);
    TextEditorLabel.attachToComponent (this, juce::Justification::top);
    TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    TextEditorLabel.setJustificationType (juce::Justification::top);
    addAndMakeVisible (TextEditorLabel);
    this->setCurrentThreadName("TextMessageLoggerThread");
}

TextMessageLoggerTextEditor::~TextMessageLoggerTextEditor()
{
    this->prepareToStop();

}

void TextMessageLoggerTextEditor::start_Thread(StringLockFreeQueue<settings::text_message_queue_size>* text_message_quePntr)
{
    text_message_queue = text_message_quePntr;
    startThread();
}

void TextMessageLoggerTextEditor::QueueDataProcessor()
{
    if (text_message_queue != nullptr)
    {
        string msg_received;
        while (text_message_queue->getNumReady() > 0)
        {
            msg_received = text_message_queue->getText();

            juce::MessageManagerLock mmlock;

            if(this->getTotalNumChars()>gui_settings::TextMessageLoggerTextEditor::maxChars)
            {
                this->clear();
            }

            if (msg_received == "clear" or msg_received == "Clear") {
                this->clear();
            }
            else
            {
                insertTextAtCaret(msg_received);
                insertTextAtCaret(juce::newLine);
            }
        }
    }

}

