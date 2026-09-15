#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>

class AudioRecorder
{
public:
    AudioRecorder() { thread.startThread(); }

    ~AudioRecorder()
    {
        stop();
        thread.stopThread (2000);
    }

    bool start (const juce::File& fileToWrite, double sampleRate, int numChannels)
    {
        stop();

        auto file = fileToWrite;
        if (file.getFileExtension().isEmpty())
            file = file.withFileExtension ("wav");

        file.getParentDirectory().createDirectory();
        if (file.existsAsFile())
            file.deleteFile();

        auto stream = file.createOutputStream();
        if (stream == nullptr || ! stream->openedOk())
            return false;

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(),
                                                                              sampleRate,
                                                                              (unsigned) juce::jmax (1, numChannels),
                                                                              24, {}, 0));
        if (writer == nullptr)
            return false;

        stream.release();

        const juce::ScopedLock lock (writerLock);
        threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (writer.release(), thread, 32768);
        recording.store (true);
        return true;
    }

    void stop()
    {
        recording.store (false);
        const juce::ScopedLock lock (writerLock);
        threadedWriter.reset();
    }

    bool isRecording() const noexcept { return recording.load(); }

    void tap (const juce::AudioBuffer<float>& buffer)
    {
        if (! recording.load())
            return;

        const juce::ScopedLock lock (writerLock);
        if (threadedWriter != nullptr)
            threadedWriter->write (buffer.getArrayOfReadPointers(), buffer.getNumSamples());
    }

private:
    juce::TimeSliceThread thread { "ANOPI Audio Rec" };
    juce::CriticalSection writerLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<bool> recording { false };
};
