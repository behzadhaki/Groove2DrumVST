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

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    bool writingActive = false;


    string latest_written_data = "";


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
        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        data[start1] = writeText;
        latest_written_data = writeText;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;
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
        num_reads += 1;

        return retrievedText;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the getText() method !!!
    string getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }

    bool isWritingInProgress()
    {
        return writingActive;
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

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    T latest_written_data;
    bool writingActive = false;

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

        writingActive = true;

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

        latest_written_data = writeData[numTowrite-1];
        num_writes = num_writes + numTowrite;
        lockFreeFifo->finishedWrite(numTowrite);
        writingActive = false;

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
        num_reads += numToRead;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    void push ( T writeData)
    {
        int start1, start2, blockSize1, blockSize2;
        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;

        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);

        writingActive = false;

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
        num_reads += 1;
        return res;
    }

    T getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;
        T readData;

        lockFreeFifo ->prepareToRead(
            getNumReady(), start1, blockSize1,
            start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            readData = *(start_data_ptr+blockSize2-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            readData = *(start_data_ptr+blockSize1-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }


        int getNumberOfReads()
        {
            return num_reads;
        }

        int getNumberOfWrites()
        {
            return num_writes;
        }

        // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
        // !! This method should only be used for initialization of GUI objects !!
        // !!! To use the QUEUE for lock free communication use the ReadFrom() or pop() methods!!!
        T getLatestDataWithoutMovingFIFOHeads()
        {
            return latest_written_data;
        }


        bool isWritingInProgress()
        {
            return writingActive;
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

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    MonotonicGroove<time_steps_> latest_written_data {};
    bool writingActive = false;


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

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

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
        num_reads += 1;
        return res;

    }

    MonotonicGroove<time_steps_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            getNumReady(), start1, blockSize1,
            start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr+blockSize2-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr+blockSize1-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    MonotonicGroove<time_steps_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};





template <int time_steps_, int num_voices_, int queue_size> class GeneratedDataQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<GeneratedData<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    GeneratedData<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;

public:
    GeneratedDataQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_GenerateData= GeneratedData<time_steps_, num_voices_>();
            data.add(empty_GenerateData);
        }
    }

    void push (const GeneratedData<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    GeneratedData<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    GeneratedData<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }

        return GeneratedData<time_steps_, num_voices_>();
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    GeneratedData<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }

    bool isWritingInProgress()
    {
        return writingActive;
    }
};



template <int time_steps_, int num_voices_, int queue_size> class HVOLightQueue
{
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<HVOLight<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    HVOLight<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;



public:

    HVOLightQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_HVOLight = HVOLight<time_steps_, num_voices_>();
            data.add(empty_HVOLight);
        }

    }

    void push (const HVOLight<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    HVOLight<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    HVOLight<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }

    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    HVOLight<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
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

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    HVO<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;


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

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

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
        num_reads += 1;
        return res;
    }

    HVO<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    HVO<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};