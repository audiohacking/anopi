#pragma once

#include "Audio/PreviewSynth.h"
#include "Chord/ChordEngine.h"
#include "Midi/ArpClock.h"
#include "Midi/ControlMap.h"
#include "Midi/LiveEventFifo.h"
#include "Midi/MidiCapture.h"
#include "Midi/ModuleRouter.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <bitset>

class AnopiAudioProcessor : public juce::AudioProcessor
{
public:
    AnopiAudioProcessor();
    ~AnopiAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.4; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    anopi::LiveEventFifo liveFifo;
    anopi::ControlMap controlMap;
    anopi::MidiCapture capture;

    anopi::ChordResult getMonitor() const;
    std::bitset<13> getHeldDegrees() const { return heldDegrees; }
    int getTonalCenter() const;
    bool pushLive (const anopi::LiveEvent& e) { return liveFifo.push (e); }
    void exportCaptureToFile (const juce::File& file);
    bool isStandaloneWrapper() const;

    std::atomic<int> lastControl { -1 };
    std::atomic<bool> shiftHeld { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    anopi::ChordRequest makeRequest() const;
    void handleIncomingMidi (const juce::MidiMessage& msg, int sample, juce::MidiBuffer& out);
    void handleLiveEvent (const anopi::LiveEvent& e, int sample, juce::MidiBuffer& out);
    void triggerDegree (int degree, int velocity, int sample, juce::MidiBuffer& out);
    void releaseDegree (int degree, int sample, juce::MidiBuffer& out);
    void refreshLinkedParts (int sample, juce::MidiBuffer& out, bool padReplace);
    void releasePad (int sample, juce::MidiBuffer& out);
    void releaseBass (int sample, juce::MidiBuffer& out);
    void strumTone (int index, bool on, int velocity, int sample, juce::MidiBuffer& out);
    void openVirtualCables();
    void sendToVirtualCables (const juce::MidiBuffer& buffer);

    anopi::ChordEngine engine;
    anopi::ModuleRouter router;
    anopi::ArpClock arp;
    anopi::PreviewSynth synth;

    std::array<std::vector<int>, 13> degreeNotes {};
    std::bitset<13> heldDegrees;
    std::vector<int> padNotes;
    std::array<int, 4> strumNotes { -1, -1, -1, -1 };
    int currentBass = -1;
    int currentAltBass = -1;
    int lastDegree = -1;
    anopi::ChordResult lastChord;
    mutable juce::SpinLock monitorLock;

    std::unique_ptr<juce::MidiOutput> cableKeys, cableBass, cableArp, cablePad;
    double currentBpm = 120.0;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnopiAudioProcessor)
};
