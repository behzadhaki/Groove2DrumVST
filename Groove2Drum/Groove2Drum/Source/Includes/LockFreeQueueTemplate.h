//
// Created by Behzad Haki on 2022-08-01.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "CustomStructs.h"

using namespace std;

/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 * to be used for Strings
 * @tparam queue_size
 */
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


/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 *  to be used for T (template) datatypes
 * @tparam T    template datatype
 * @tparam queue_size
 */
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
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        return res;
    }
};


/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 * to be used for CustomStructs::MonotonicGroove datatypes
 * @tparam time_steps_
 * @tparam queue_size
 */
template <int time_steps_, int queue_size> class MonotonicGrooveQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<MonotonicGroove<time_steps_>> data{};

    int time_steps;
public:
    MonotonicGrooveQueue(){

        time_steps = time_steps_;

        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_groove = MonotonicGroove<time_steps_>();
            data.add(empty_groove);
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
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        return res;

    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }

};


/***
  * a Juce::abstractfifo implementation of a Lockfree FIFO
  * to be used for CustomStructs::HVO datatypes
  * @tparam time_steps_
  * @tparam num_voices_
  * @tparam queue_size
  */
template <int time_steps_, int num_voices_, int queue_size> class HVOQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<HVO<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

public:
    HVOQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_HVO = HVO<time_steps_, num_voices_>();
            data.add(empty_HVO);
        }

    }
    void push (const HVO<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        lockFreeFifo->finishedWrite(1);
    }

    HVO<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        return res;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }
};