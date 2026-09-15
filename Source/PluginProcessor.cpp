#include "PluginProcessor.h"

namespace
{
juce::StringArray keyNames() { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
}

juce::AudioProcessorValueTreeState::ParameterLayout AnopiAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("tonalCenter", "Tonal Center", keyNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("tonality", "Minor", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("scale", "Scale",
        juce::StringArray { "Major", "Minor", "Harm. minor", "Mel. minor", "Dorian",
                            "Phrygian", "Lydian", "Mixolydian", "Locrian", "Harm. major" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("color", "Color",
        juce::StringArray { "Diatonic", "add6", "6/9", "Quartal" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("inversion", "Inversion",
        juce::StringArray { "Root", "1st", "2nd", "3rd" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("layout", "Real Scale", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("shift", "Shift", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("extensions", "Extensions",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.4f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("voicing", "Voicing",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.45f));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("sustain", "Sustain", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("keysOn", "Keys", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("bassOn", "Bass", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("arpOn", "Arp", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("padOn", "Pad", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("bassLink", "Bass Link", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("padLatch", "Pad Latch", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("arpDivision", "Arp Division",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("outputMode", "Output Mode",
        juce::StringArray { "Channels", "Cables" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("inputOctave", "Play Octave",
        juce::StringArray { "1", "2", "3", "4", "5", "6" }, 2));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("tonalOctave", "Tonal Octave",
        juce::StringArray { "2", "3", "4", "5", "6", "7" }, 3));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("previewSynth", "Internal Tones", true));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("masterGain", "Tone Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("capture", "Capture", false));
    return { p.begin(), p.end() };
}

AnopiAudioProcessor::AnopiAudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ANOPI", createLayout())
{
}

bool AnopiAudioProcessor::isStandaloneWrapper() const
{
    return wrapperType == wrapperType_Standalone;
}

int AnopiAudioProcessor::getTonalCenter() const
{
    return (int) apvts.getRawParameterValue ("tonalCenter")->load();
}

anopi::ChordResult AnopiAudioProcessor::getMonitor() const
{
    const juce::SpinLock::ScopedLockType lock (monitorLock);
    return lastChord;
}

anopi::ChordRequest AnopiAudioProcessor::makeRequest() const
{
    anopi::ChordRequest r;
    r.tonalCenter = (int) apvts.getRawParameterValue ("tonalCenter")->load();
    r.scale = (anopi::ScaleId) juce::jlimit (0, (int) anopi::ScaleId::Count - 1,
                                            (int) apvts.getRawParameterValue ("scale")->load());
    r.tonality = (r.scale == anopi::ScaleId::NaturalMinor || r.scale == anopi::ScaleId::HarmonicMinor
                  || r.scale == anopi::ScaleId::MelodicMinor)
                     ? anopi::Tonality::Minor
                     : anopi::Tonality::Major;
    r.color = (anopi::ColorMode) juce::jlimit (0, (int) anopi::ColorMode::Count - 1,
                                              (int) apvts.getRawParameterValue ("color")->load());
    r.inversion = juce::jlimit (0, 3, (int) apvts.getRawParameterValue ("inversion")->load());
    r.layout = apvts.getRawParameterValue ("layout")->load() > 0.5f ? anopi::LayoutMode::RealScale
                                                                    : anopi::LayoutMode::Static;
    r.shift = apvts.getRawParameterValue ("shift")->load() > 0.5f || shiftHeld.load();
    r.extensions = apvts.getRawParameterValue ("extensions")->load();
    r.voicing = apvts.getRawParameterValue ("voicing")->load();
    return r;
}

void AnopiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    router.prepare();
    arp.prepare (sampleRate);
    synth.prepare (sampleRate, samplesPerBlock);
    capture.prepare (sampleRate);
    openVirtualCables();
}

void AnopiAudioProcessor::releaseResources()
{
    cableKeys.reset();
    cableBass.reset();
    cableArp.reset();
    cablePad.reset();
    synth.reset();
}

bool AnopiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    return main == juce::AudioChannelSet::mono() || main == juce::AudioChannelSet::stereo();
}

void AnopiAudioProcessor::openVirtualCables()
{
    if (! isStandaloneWrapper())
        return;
    if (apvts.getRawParameterValue ("outputMode")->load() < 0.5f)
        return;

    if (cableKeys == nullptr) cableKeys = juce::MidiOutput::createNewDevice ("ANOPI Keys");
    if (cableBass == nullptr) cableBass = juce::MidiOutput::createNewDevice ("ANOPI Bass");
    if (cableArp == nullptr)  cableArp  = juce::MidiOutput::createNewDevice ("ANOPI Arp");
    if (cablePad == nullptr)  cablePad  = juce::MidiOutput::createNewDevice ("ANOPI Pad");
}

void AnopiAudioProcessor::sendToVirtualCables (const juce::MidiBuffer& buffer)
{
    if (cableKeys == nullptr && cableBass == nullptr && cableArp == nullptr && cablePad == nullptr)
        return;

    juce::MidiBuffer b0, b1, b2, b3;
    const int ch[4] = { router.getChannel (anopi::Module::Keys), router.getChannel (anopi::Module::Bass),
                        router.getChannel (anopi::Module::Arp), router.getChannel (anopi::Module::Pad) };
    juce::MidiBuffer* outs[4] = { &b0, &b1, &b2, &b3 };
    juce::MidiOutput* cables[4] = { cableKeys.get(), cableBass.get(), cableArp.get(), cablePad.get() };

    for (const auto meta : buffer)
    {
        const auto msg = meta.getMessage();
        for (int i = 0; i < 4; ++i)
            if (msg.getChannel() == ch[i])
                outs[i]->addEvent (msg, meta.samplePosition);
    }

    for (int i = 0; i < 4; ++i)
        if (cables[i] != nullptr && ! outs[i]->isEmpty())
            cables[i]->sendBlockOfMessagesNow (*outs[i]);
}

void AnopiAudioProcessor::triggerDegree (int degree, int velocity, int sample, juce::MidiBuffer& out)
{
    degree = juce::jlimit (0, 12, degree);
    auto req = makeRequest();
    req.degreeKeyPc = degree;
    auto chord = engine.resolve (req);

    {
        const juce::SpinLock::ScopedLockType lock (monitorLock);
        lastChord = chord;
    }

    releaseDegree (degree, sample, out);
    heldDegrees.set ((size_t) degree);
    lastDegree = degree;
    degreeNotes[(size_t) degree] = chord.midiNotes;

    for (int n : chord.midiNotes)
        router.noteOn (anopi::Module::Keys, n, velocity, sample, out);

    refreshLinkedParts (sample, out, true);
    arp.setChord (chord.midiNotes);
}

void AnopiAudioProcessor::releaseDegree (int degree, int sample, juce::MidiBuffer& out)
{
    degree = juce::jlimit (0, 12, degree);
    for (int n : degreeNotes[(size_t) degree])
        router.noteOff (anopi::Module::Keys, n, sample, out);
    degreeNotes[(size_t) degree].clear();
    heldDegrees.reset ((size_t) degree);

    if (heldDegrees.none() && apvts.getRawParameterValue ("padLatch")->load() < 0.5f
        && apvts.getRawParameterValue ("sustain")->load() < 0.5f)
        releasePad (sample, out);

    if (heldDegrees.none() && apvts.getRawParameterValue ("bassLink")->load() > 0.5f
        && apvts.getRawParameterValue ("sustain")->load() < 0.5f)
        releaseBass (sample, out);
}

void AnopiAudioProcessor::refreshLinkedParts (int sample, juce::MidiBuffer& out, bool padReplace)
{
    const bool bassLink = apvts.getRawParameterValue ("bassLink")->load() > 0.5f;
    if (bassLink && lastChord.bassNote >= 0)
    {
        if (currentBass != lastChord.bassNote)
        {
            releaseBass (sample, out);
            currentBass = lastChord.bassNote;
            router.noteOn (anopi::Module::Bass, currentBass, 100, sample, out);
        }
    }

    if (padReplace && lastChord.midiNotes.size() > 0)
    {
        releasePad (sample, out);
        padNotes = lastChord.midiNotes;
        for (int n : padNotes)
            router.noteOn (anopi::Module::Pad, n, 82, sample, out);
    }
}

void AnopiAudioProcessor::releasePad (int sample, juce::MidiBuffer& out)
{
    for (int n : padNotes)
        router.noteOff (anopi::Module::Pad, n, sample, out);
    padNotes.clear();
}

void AnopiAudioProcessor::releaseBass (int sample, juce::MidiBuffer& out)
{
    if (currentBass >= 0)
        router.noteOff (anopi::Module::Bass, currentBass, sample, out);
    if (currentAltBass >= 0)
        router.noteOff (anopi::Module::Bass, currentAltBass, sample, out);
    currentBass = -1;
    currentAltBass = -1;
}

void AnopiAudioProcessor::strumTone (int index, bool on, int velocity, int sample, juce::MidiBuffer& out)
{
    index = juce::jlimit (0, 3, index);
    if (! on)
    {
        if (strumNotes[(size_t) index] >= 0)
            router.noteOff (anopi::Module::Keys, strumNotes[(size_t) index], sample, out);
        strumNotes[(size_t) index] = -1;
        return;
    }

    if (lastChord.midiNotes.empty())
        return;

    const int note = lastChord.midiNotes[(size_t) juce::jmin (index, (int) lastChord.midiNotes.size() - 1)];
    if (strumNotes[(size_t) index] >= 0)
        router.noteOff (anopi::Module::Keys, strumNotes[(size_t) index], sample, out);
    strumNotes[(size_t) index] = note;
    router.noteOn (anopi::Module::Keys, note, velocity, sample, out);
}

void AnopiAudioProcessor::handleLiveEvent (const anopi::LiveEvent& e, int sample, juce::MidiBuffer& out)
{
    using T = anopi::LiveEvent::Type;
    switch (e.type)
    {
        case T::DegreeOn:  triggerDegree ((int) e.a, (int) e.b, sample, out); break;
        case T::DegreeOff: releaseDegree ((int) e.a, sample, out); break;
        case T::StrumOn:   strumTone ((int) e.a, true, (int) e.b, sample, out); break;
        case T::StrumOff:  strumTone ((int) e.a, false, 0, sample, out); break;
        case T::BassRootOn:
            if (lastChord.bassNote >= 0)
            {
                if (currentBass >= 0)
                    router.noteOff (anopi::Module::Bass, currentBass, sample, out);
                currentBass = lastChord.bassNote;
                router.noteOn (anopi::Module::Bass, currentBass, (int) e.b, sample, out);
            }
            break;
        case T::BassRootOff:
            if (currentBass >= 0)
                router.noteOff (anopi::Module::Bass, currentBass, sample, out);
            currentBass = -1;
            break;
        case T::BassAltOn:
            if (lastChord.altBassNote >= 0)
            {
                if (currentAltBass >= 0)
                    router.noteOff (anopi::Module::Bass, currentAltBass, sample, out);
                currentAltBass = lastChord.altBassNote;
                router.noteOn (anopi::Module::Bass, currentAltBass, (int) e.b, sample, out);
            }
            break;
        case T::BassAltOff:
            if (currentAltBass >= 0)
                router.noteOff (anopi::Module::Bass, currentAltBass, sample, out);
            currentAltBass = -1;
            break;
        case T::PitchBend:
            router.pitchBend (e.extra, sample, out);
            break;
        case T::AllNotesOff:
            router.allNotesOff (sample, out);
            heldDegrees.reset();
            for (auto& v : degreeNotes) v.clear();
            padNotes.clear();
            currentBass = currentAltBass = -1;
            arp.reset();
            synth.reset();
            break;
    }
}

void AnopiAudioProcessor::handleIncomingMidi (const juce::MidiMessage& msg, int sample, juce::MidiBuffer& out)
{
    const int playOct = (int) apvts.getRawParameterValue ("inputOctave")->load() + 1;
    const int tonalOct = (int) apvts.getRawParameterValue ("tonalOctave")->load() + 2;
    const int playBase = playOct * 12;
    const int tonalBase = tonalOct * 12;

    if (msg.isNoteOnOrOff())
    {
        const int note = msg.getNoteNumber();
        const int vel = msg.isNoteOn() ? msg.getVelocity() : 0;

        if (note >= playBase && note <= playBase + 12)
        {
            const int degree = note - playBase;
            if (msg.isNoteOn() && vel > 0)
                triggerDegree (degree, vel, sample, out);
            else
                releaseDegree (degree, sample, out);
            lastControl.store (degree);
            return;
        }

        if (note >= tonalBase && note < tonalBase + 12)
        {
            if (msg.isNoteOn())
                if (auto* p = apvts.getParameter ("tonalCenter"))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) (note - tonalBase)));
            return;
        }
    }

    if (msg.isSustainPedalOn() || msg.isSustainPedalOff())
    {
        const bool on = msg.isSustainPedalOn();
        if (auto* p = apvts.getParameter ("sustain"))
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        router.setSustain (on, sample, out);
        return;
    }

    if (msg.isPitchWheel())
    {
        router.pitchBend (msg.getPitchWheelValue(), sample, out);
        return;
    }

    if (msg.isController())
    {
        const int cc = msg.getControllerNumber();
        const int val = msg.getControllerValue();
        const float n = (float) val / 127.0f;

        if (controlMap.consumeLearnCC (cc, msg.getChannel()))
            return;

        auto applyBool = [&] (const char* id, bool toggle)
        {
            if (auto* p = apvts.getParameter (id))
            {
                if (toggle)
                    p->setValueNotifyingHost (p->getValue() < 0.5f ? 1.0f : 0.0f);
                else
                    p->setValueNotifyingHost (n > 0.5f ? 1.0f : 0.0f);
            }
        };

        if (cc == controlMap.extensions.cc)
            apvts.getParameter ("extensions")->setValueNotifyingHost (n);
        else if (cc == controlMap.voicing.cc)
            apvts.getParameter ("voicing")->setValueNotifyingHost (n);
        else if (cc == controlMap.tonality.cc)
            applyBool ("tonality", val == 127);
        else if (cc == controlMap.layout.cc)
            applyBool ("layout", val == 127);
        else if (cc == controlMap.muteKeys.cc)
            applyBool ("keysOn", true);
        else if (cc == controlMap.muteBass.cc)
            applyBool ("bassOn", true);
        else if (cc == controlMap.muteArp.cc)
            applyBool ("arpOn", true);
        else if (cc == controlMap.mutePad.cc)
            applyBool ("padOn", true);
        else if (cc == controlMap.sustain.cc)
        {
            applyBool ("sustain", false);
            router.setSustain (n > 0.5f, sample, out);
        }
    }
}

void AnopiAudioProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBpm = *bpm;

    juce::MidiBuffer incoming;
    incoming.swapWith (midi);
    midi.clear();

    const bool keysOn = apvts.getRawParameterValue ("keysOn")->load() > 0.5f;
    const bool bassOn = apvts.getRawParameterValue ("bassOn")->load() > 0.5f;
    const bool arpOn = apvts.getRawParameterValue ("arpOn")->load() > 0.5f;
    const bool padOn = apvts.getRawParameterValue ("padOn")->load() > 0.5f;
    router.setEnabled (anopi::Module::Keys, keysOn, midi, 0);
    router.setEnabled (anopi::Module::Bass, bassOn, midi, 0);
    router.setEnabled (anopi::Module::Arp, arpOn, midi, 0);
    router.setEnabled (anopi::Module::Pad, padOn, midi, 0);
    router.setSustain (apvts.getRawParameterValue ("sustain")->load() > 0.5f, 0, midi);

    capture.setRecording (apvts.getRawParameterValue ("capture")->load() > 0.5f);

    if (apvts.getRawParameterValue ("outputMode")->load() > 0.5f)
        openVirtualCables();

    for (const auto meta : incoming)
        handleIncomingMidi (meta.getMessage(), meta.samplePosition, midi);

    anopi::LiveEvent live;
    while (liveFifo.pop (live))
        handleLiveEvent (live, 0, midi);

    const int divChoice = (int) apvts.getRawParameterValue ("arpDivision")->load();
    const int division = 4 << juce::jlimit (0, 3, divChoice); // 4,8,16,32
    arp.process ((int) audio.getNumSamples(), currentBpm, division, arpOn,
                 [&] (int note, int sample) { router.noteOn (anopi::Module::Arp, note, 96, sample, midi); },
                 [&] (int note, int sample) { router.noteOff (anopi::Module::Arp, note, sample, midi); });

    const std::array<int, 4> chans {
        router.getChannel (anopi::Module::Keys), router.getChannel (anopi::Module::Bass),
        router.getChannel (anopi::Module::Arp), router.getChannel (anopi::Module::Pad)
    };
    capture.tap (midi, audio.getNumSamples(), chans);
    sendToVirtualCables (midi);

    const bool tones = apvts.getRawParameterValue ("previewSynth")->load() > 0.5f;
    const float gain = apvts.getRawParameterValue ("masterGain")->load();
    if (tones)
    {
        synth.handleMidi (midi);
        synth.render (audio, gain);
    }
    else
    {
        audio.clear();
    }
}

void AnopiAudioProcessor::exportCaptureToFile (const juce::File& file)
{
    auto midiFile = capture.toMidiFile (currentBpm);
    if (file.existsAsFile())
        file.deleteFile();
    juce::FileOutputStream stream (file);
    if (stream.openedOk())
        midiFile.writeTo (stream);
}

void AnopiAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AnopiAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnopiAudioProcessor();
}
