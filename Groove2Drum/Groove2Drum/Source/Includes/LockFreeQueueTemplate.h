//
// Created by Behzad Haki on 2022-08-01.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "Representations.h"

using namespace std;

template<int queue_size> class StringLockFreeQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    unique_ptr<juce::AbstractFifo> lockFreeFifo;
    string data[queue_size];

public:
    StringLockFreeQueue()
    {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));
    }

    void addText (string writeText)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        data[start1] = writeText;

        lockFreeFifo->finishedWrite(1);
    }

    string getText()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        string retrievedText;
        retrievedText = data[start1];

        lockFreeFifo -> finishedRead(1);

        return retrievedText;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


};

template <typename T, int queue_size> class LockFreeQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<T> data;

public:
    LockFreeQueue()
    {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

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


    void push (const T writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        lockFreeFifo->finishedWrite(1);
    }

    T pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        return *(start_data_ptr);
    }
};


template <int time_steps_, int queue_size> class MonotonicGrooveQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<MonotonicGroove<time_steps_>> data{};

public:
    MonotonicGrooveQueue(){
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_note = MonotonicGroove<time_steps_>();
            data.add(empty_note);
        }

    }

    void push (const MonotonicGroove<time_steps_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        lockFreeFifo->finishedWrite(1);
    }

    MonotonicGroove<time_steps_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        return *(start_data_ptr);
    }

};