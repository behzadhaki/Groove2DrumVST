//
// Created by Behzad Haki on 2022-08-01.
//

#pragma once

#include "../PluginProcessor.h"
#include "Representations.h"

template <typename T, int queue_size> class LockFreeQueue
{
private:
    juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;
    juce::Array<T> data;

public:
    LockFreeQueue()
    {
        lockFreeFifo = new juce::AbstractFifo(queue_size);

        data.ensureStorageAllocated(queue_size);

        /*while (data.size() < settings::note_queue_size)
        {
            auto empty_note = Note(
                0, 0, 0, 0, 0);
            data.add(empty_note);
        }*/

    }

    void WriteTo (const T* writeData, int numTowrite)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToWrite(
            numTowrite, start1, blockSize1,
            start2, blockSize2);

        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;

            for (int i=0; i < blockSize1; i++)
            {
                *(start_data_ptr + i) = *(writeData+i);
            }
        }

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            for (int i=0; i < blockSize2; i++)
            {
                *(start_data_ptr + i) = *(writeData+blockSize1+i);
            }
        }

        lockFreeFifo->finishedWrite(numTowrite);
    }

    void ReadFrom(T* readData, int numToRead)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            numToRead, start1, blockSize1,
            start2, blockSize2);

        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            for (int i=0; i < blockSize1; i++)
            {
                *(readData + i) = *(start_data_ptr+i);
            }
        }

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            for (int i=0; i < blockSize2; i++)
            {
                *(readData+blockSize1+i) = *(start_data_ptr + i);
            }
        }
        lockFreeFifo -> finishedRead(blockSize1+blockSize2);
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


};