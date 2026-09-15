#include "PluginEditor.h"
#include <cmath>

namespace
{
void styleToggle (juce::ToggleButton& b)
{
    b.setClickingTogglesState (true);
}
} // namespace

AnopiAudioProcessorEditor::AnopiAudioProcessorEditor (AnopiAudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    setWantsKeyboardFocus (true);
    setOpaque (true);
    setSize (1080, 680);
    setResizable (true, true);
    setResizeLimits (960, 580, 1600, 940);

    addAndMakeVisible (monitor);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (tonal);
    addAndMakeVisible (strum);
    addAndMakeVisible (scalePads);
    addAndMakeVisible (colorPads);
    addAndMakeVisible (invPads);
    addAndMakeVisible (arpPads);
    addAndMakeVisible (outPads);
    addAndMakeVisible (octPads);
    addAndMakeVisible (extPads);

    addAndMakeVisible (voicing);
    addAndMakeVisible (bend);
    addAndMakeVisible (gain);
    bend.setRange (0.0, 1.0, 0.0001);
    bend.setValue (0.5, juce::dontSendNotification);

    for (auto* b : { &staticReal, &shift, &sustain, &keysOn, &bassOn, &arpOn, &padOn,
                     &bassLink, &padLatch, &preview, &qwertyHelp })
    {
        styleToggle (*b);
        addAndMakeVisible (*b);
    }

    addAndMakeVisible (captureBtn);
    addAndMakeVisible (recBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (loadBtn);
    addAndMakeVisible (allOffBtn);
    addAndMakeVisible (learnBtn);

    recBtn.setVisible (true);
    recBtn.setEnabled (proc.isStandaloneWrapper());
    recBtn.setTooltip (proc.isStandaloneWrapper() ? "Record preview audio to WAV" : "Audio REC is Standalone only");
    staticReal.setTooltip ("Real scale layout");
    lastMidiFolder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    lastWavFolder = lastMidiFolder;
    lastSettingsFolder = proc.getSettingsFolder();

    voiceLabel.setJustificationType (juce::Justification::centred);
    bendLabel.setJustificationType (juce::Justification::centred);
    footer.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (voiceLabel);
    addAndMakeVisible (bendLabel);
    addAndMakeVisible (footer);

    keyboard.onDegree = [this] (int d, bool on) { pushDegree (d, on); };
    tonal.onSelect = [this] (int pc)
    {
        if (auto* param = proc.apvts.getParameter ("tonalCenter"))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) pc));
    };
    strum.onStrum = [this] (int i, bool on)
    {
        proc.pushLive ({ on ? anopi::LiveEvent::Type::StrumOn : anopi::LiveEvent::Type::StrumOff,
                         (uint8_t) i, (uint8_t) 100, 0 });
    };

    bend.onValueChange = [this]
    {
        const int v = (int) std::lround (bend.getValue() * 16383.0);
        if (v != lastBend)
        {
            lastBend = v;
            proc.pushLive ({ anopi::LiveEvent::Type::PitchBend, 0, 0, v });
        }
    };

    captureBtn.onClick = [this]
    {
        if (proc.capture.isRecording())
        {
            auto dest = pendingMidiFile;
            setCaptureArmed (false);
            if (dest != juce::File {})
                proc.exportCaptureToFile (dest);
        }
        else
        {
            chooseAndStartMidiCapture();
        }
    };

    recBtn.onClick = [this]
    {
        if (proc.isAudioCapturing())
        {
            proc.stopAudioCapture();
            setRecButtonRecording (false);
        }
        else
        {
            chooseAndStartAudioCapture();
        }
    };

    saveBtn.onClick = [this] { chooseAndSaveSettings(); };
    loadBtn.onClick = [this] { chooseAndLoadSettings(); };

    allOffBtn.onClick = [this]
    {
        proc.pushLive ({ anopi::LiveEvent::Type::AllNotesOff, 0, 0, 0 });
    };

    learnBtn.onClick = [this]
    {
        proc.controlMap.armLearn (0);
        learnBtn.setButtonText ("Move a CC…");
    };

    qwertyHelp.onClick = [this]
    {
        showMap = qwertyHelp.getToggleState();
        footer.setText (showMap
                            ? "Arrows: SCALE grid (← darker → brighter, ↑↓ parallel). ; ' voicing. [ ] extensions."
                            : "Arrows: SCALE grid    [ ] ext    O color    I inv    Shift 2nd chord",
                        juce::dontSendNotification);
    };

    voiceAt = std::make_unique<FAttach> (proc.apvts, "voicing", voicing);
    gainAt = std::make_unique<FAttach> (proc.apvts, "masterGain", gain);
    layAt = std::make_unique<BAttach> (proc.apvts, "layout", staticReal);
    shiftAt = std::make_unique<BAttach> (proc.apvts, "shift", shift);
    susAt = std::make_unique<BAttach> (proc.apvts, "sustain", sustain);
    kAt = std::make_unique<BAttach> (proc.apvts, "keysOn", keysOn);
    bAt = std::make_unique<BAttach> (proc.apvts, "bassOn", bassOn);
    aAt = std::make_unique<BAttach> (proc.apvts, "arpOn", arpOn);
    pAt = std::make_unique<BAttach> (proc.apvts, "padOn", padOn);
    linkAt = std::make_unique<BAttach> (proc.apvts, "bassLink", bassLink);
    latchAt = std::make_unique<BAttach> (proc.apvts, "padLatch", padLatch);
    prevAt = std::make_unique<BAttach> (proc.apvts, "previewSynth", preview);

    auto bindPads = [this] (PadBank& bank, const char* paramId)
    {
        bank.onSelect = [this, paramId] (int i) { setChoice (paramId, i); };
    };
    bindPads (scalePads, "scale");
    bindPads (colorPads, "color");
    bindPads (invPads, "inversion");
    bindPads (arpPads, "arpDivision");
    bindPads (outPads, "outputMode");
    bindPads (octPads, "inputOctave");
    extPads.onSelect = [this] (int stage)
    {
        static constexpr float kStageValue[] = { 0.09f, 0.30f, 0.50f, 0.65f, 0.79f, 0.93f };
        if (auto* p = proc.apvts.getParameter ("extensions"))
            p->setValueNotifyingHost (kStageValue[juce::jlimit (0, 5, stage)]);
        grabKeyboardFocus();
    };

    for (auto* c : getChildren())
        c->setWantsKeyboardFocus (false);
    setWantsKeyboardFocus (true);

    startTimerHz (30);
    juce::Timer::callAfterDelay (150, [this] { grabKeyboardFocus(); });
}

AnopiAudioProcessorEditor::~AnopiAudioProcessorEditor()
{
    auto dest = pendingMidiFile;
    setCaptureArmed (false);
    if (dest != juce::File {})
        proc.exportCaptureToFile (dest);
    proc.stopAudioCapture();
    setLookAndFeel (nullptr);
}

void AnopiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffc5d4b8));
    g.setColour (juce::Colour (0xff2a3328).withAlpha (0.12f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (6.0f), 18.0f, 2.0f);

    g.setColour (juce::Colour (0xff2a3328));
    g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    g.drawText ("ANOPI", 12, 8, 84, 22, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (juce::Colour (0xff5a6658));
    g.drawText ("chord builder", 96, 10, 100, 18, juce::Justification::centredLeft);
}

void AnopiAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto top = r.removeFromTop (22);
    top.removeFromLeft (148);

    auto placeRow = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> items,
                        std::initializer_list<float> weights)
    {
        float sum = 0.0f;
        for (auto w : weights)
            sum += w;
        auto wit = weights.begin();
        for (auto* c : items)
        {
            const float w = *wit++;
            const int px = juce::jmax (30, (int) std::lround ((float) row.getWidth() * (w / juce::jmax (0.001f, sum))));
            c->setBounds (row.removeFromLeft (juce::jmin (px, row.getWidth())).reduced (1, 0));
        }
    };

    auto file = top.removeFromRight (juce::jmax (236, top.getWidth() / 3));
    placeRow (file, { &captureBtn, &recBtn, &saveBtn, &loadBtn, &allOffBtn },
              { 1.3f, 0.85f, 0.9f, 0.9f, 1.0f });
    top.removeFromRight (4);
    preview.setBounds (top.removeFromRight (92).reduced (1, 0));
    top.removeFromRight (4);
    placeRow (top, { &keysOn, &bassOn, &arpOn, &padOn, &shift, &staticReal, &sustain },
              { 1.0f, 1.0f, 0.85f, 0.85f, 1.05f, 0.9f, 1.15f });

    r.removeFromTop (6);
    auto harmony = r.removeFromTop (76);
    scalePads.setBounds (harmony.removeFromLeft ((int) (harmony.getWidth() * 0.50f)).reduced (2, 0));
    colorPads.setBounds (harmony.removeFromLeft ((int) (harmony.getWidth() * 0.50f)).reduced (2, 0));
    invPads.setBounds (harmony.reduced (2, 0));

    r.removeFromTop (4);
    auto footerR = r.removeFromBottom (22);
    footer.setBounds (footerR.removeFromLeft (r.getWidth() - 340));
    qwertyHelp.setBounds (footerR.removeFromLeft (92).reduced (1, 0));
    learnBtn.setBounds (footerR.removeFromLeft (100).reduced (1, 0));
    gain.setBounds (footerR.reduced (2, 3));

    auto sliders = r.removeFromRight (72);
    voiceLabel.setBounds (sliders.removeFromTop (16));
    voicing.setBounds (sliders.removeFromTop (sliders.getHeight() / 2).reduced (20, 2));
    bendLabel.setBounds (sliders.removeFromTop (16));
    bend.setBounds (sliders.reduced (20, 2));

    auto mid = r.removeFromTop (88);
    monitor.setBounds (mid.removeFromLeft ((int) (mid.getWidth() * 0.58f)).reduced (2));
    extPads.setBounds (mid.reduced (2));

    auto bottom = r;
    keyboard.setBounds (bottom.removeFromLeft ((int) (bottom.getWidth() * 0.48f)).reduced (2));
    auto right = bottom;
    tonal.setBounds (right.removeFromTop ((int) (right.getHeight() * 0.38f)).reduced (2));
    auto row = right.reduced (2);
    strum.setBounds (row.removeFromLeft ((int) (row.getWidth() * 0.40f)));
    auto opts = row.reduced (2, 0);
    auto toggles = opts.removeFromTop (26);
    bassLink.setBounds (toggles.removeFromLeft (toggles.getWidth() / 2).reduced (1, 0));
    padLatch.setBounds (toggles.reduced (1, 0));
    arpPads.setBounds (opts.removeFromTop ((int) (opts.getHeight() * 0.34f)).reduced (0, 1));
    outPads.setBounds (opts.removeFromTop ((int) (opts.getHeight() * 0.45f)).reduced (0, 1));
    octPads.setBounds (opts.reduced (0, 1));
}

void AnopiAudioProcessorEditor::timerCallback()
{
    const auto chord = proc.getMonitor();
    monitor.setChord (chord);
    keyboard.setHeld (proc.getHeldDegrees());
    const auto scaleId = (anopi::ScaleId) juce::jlimit (0, (int) anopi::ScaleId::Count - 1,
                                                       (int) proc.apvts.getRawParameterValue ("scale")->load());
    keyboard.setLabels (anopi::degreeLabels (scaleId));
    scalePads.setSelected ((int) scaleId);
    colorPads.setSelected (juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("color")->load()));
    invPads.setSelected (juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("inversion")->load()));
    arpPads.setSelected (juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("arpDivision")->load()));
    outPads.setSelected (juce::jlimit (0, 1, (int) proc.apvts.getRawParameterValue ("outputMode")->load()));
    octPads.setSelected (juce::jlimit (0, 5, (int) proc.apvts.getRawParameterValue ("inputOctave")->load()));
    extPads.setSelected ((int) anopi::stageFromExtensions (proc.apvts.getRawParameterValue ("extensions")->load()));
    tonal.setSelected (proc.getTonalCenter());
    strum.setTones (chord.midiNotes);

    if (proc.controlMap.learnTarget.load() < 0 && learnBtn.getButtonText() != "MIDI learn")
        learnBtn.setButtonText ("MIDI learn");

    recBtn.setEnabled (proc.isStandaloneWrapper());
    recBtn.setTooltip (proc.isStandaloneWrapper() ? "Record preview audio to WAV" : "Audio REC is Standalone only");
    if (recBtn.isEnabled())
        setRecButtonRecording (proc.isAudioCapturing());
}

void AnopiAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    grabKeyboardFocus();
}

void AnopiAudioProcessorEditor::pushDegree (int degree, bool on)
{
    proc.pushLive ({ on ? anopi::LiveEvent::Type::DegreeOn : anopi::LiveEvent::Type::DegreeOff,
                     (uint8_t) juce::jlimit (0, 12, degree), (uint8_t) 100, 0 });
}

void AnopiAudioProcessorEditor::setChoice (const char* paramId, int index)
{
    if (auto* p = proc.apvts.getParameter (paramId))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    grabKeyboardFocus();
}

void AnopiAudioProcessorEditor::setCaptureArmed (bool on)
{
    if (auto* p = proc.apvts.getParameter ("capture"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
    captureBtn.setButtonText (on ? "STOP" : "Capture");
    if (! on)
        pendingMidiFile = juce::File();
}

void AnopiAudioProcessorEditor::setRecButtonRecording (bool on)
{
    recBtn.setButtonText (on ? "STOP" : "REC");
}

void AnopiAudioProcessorEditor::chooseAndStartMidiCapture()
{
    auto startAt = lastMidiFolder.getChildFile ("ANOPI.mid");
    auto chooser = std::make_shared<juce::FileChooser> ("Save MIDI capture", startAt, "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File {})
                                  return;
                              if (file.getFileExtension().isEmpty())
                                  file = file.withFileExtension ("mid");
                              lastMidiFolder = file.getParentDirectory();
                              pendingMidiFile = file;
                              setCaptureArmed (true);
                              grabKeyboardFocus();
                          });
}

void AnopiAudioProcessorEditor::chooseAndStartAudioCapture()
{
    if (! proc.isStandaloneWrapper())
        return;

    auto startAt = lastWavFolder.getChildFile ("ANOPI.wav");
    auto chooser = std::make_shared<juce::FileChooser> ("Record audio", startAt, "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File {})
                                  return;
                              if (file.getFileExtension().isEmpty())
                                  file = file.withFileExtension ("wav");
                              lastWavFolder = file.getParentDirectory();
                              if (proc.startAudioCapture (file))
                                  setRecButtonRecording (true);
                              grabKeyboardFocus();
                          });
}

void AnopiAudioProcessorEditor::chooseAndSaveSettings()
{
    auto startAt = lastSettingsFolder.getChildFile ("ANOPI.xml");
    auto chooser = std::make_shared<juce::FileChooser> ("Save ANOPI settings", startAt, "*.xml");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File {})
                                  return;
                              if (file.getFileExtension().isEmpty())
                                  file = file.withFileExtension ("xml");
                              lastSettingsFolder = file.getParentDirectory();
                              proc.saveSettingsToFile (file);
                              grabKeyboardFocus();
                          });
}

void AnopiAudioProcessorEditor::chooseAndLoadSettings()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load ANOPI settings", lastSettingsFolder, "*.xml");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File {})
                                  return;
                              lastSettingsFolder = file.getParentDirectory();
                              proc.loadSettingsFromFile (file);
                              grabKeyboardFocus();
                          });
}

void AnopiAudioProcessorEditor::syncQwertyDegrees()
{
    for (int i = 0; i < 13; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown (anopi::ControlMap::kDegreeKeyCodes[(size_t) i]);
        if (down != qwertyHeld.test ((size_t) i))
        {
            qwertyHeld.set ((size_t) i, down);
            pushDegree (i, down);
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown (anopi::ControlMap::kStrumKeyCodes[(size_t) i]);
        if (down != qwertyStrum.test ((size_t) i))
        {
            qwertyStrum.set ((size_t) i, down);
            proc.pushLive ({ down ? anopi::LiveEvent::Type::StrumOn : anopi::LiveEvent::Type::StrumOff,
                             (uint8_t) i, (uint8_t) 100, 0 });
        }
    }

    const bool root = juce::KeyPress::isKeyCurrentlyDown ('B');
    if (root != qwertyBassRoot)
    {
        qwertyBassRoot = root;
        proc.pushLive ({ root ? anopi::LiveEvent::Type::BassRootOn : anopi::LiveEvent::Type::BassRootOff, 0, 100, 0 });
    }
    const bool alt = juce::KeyPress::isKeyCurrentlyDown ('N');
    if (alt != qwertyBassAlt)
    {
        qwertyBassAlt = alt;
        proc.pushLive ({ alt ? anopi::LiveEvent::Type::BassAltOn : anopi::LiveEvent::Type::BassAltOff, 0, 100, 0 });
    }

    proc.shiftHeld.store (juce::ModifierKeys::getCurrentModifiers().isShiftDown());
}

bool AnopiAudioProcessorEditor::keyStateChanged (bool)
{
    syncQwertyDegrees();
    return false;
}

bool AnopiAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
        return false;

    const int code = key.getKeyCode();

    if (auto pc = anopi::ControlMap::qwertyTonal (code))
    {
        if (auto* param = proc.apvts.getParameter ("tonalCenter"))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) *pc));
        return true;
    }

    auto toggle = [&] (const char* id)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (p->getValue() < 0.5f ? 1.0f : 0.0f);
    };

    auto step = [&] (const char* id, float delta)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + delta));
    };

    auto cycle = [&] (const char* id, int count, int delta)
    {
        if (auto* p = proc.apvts.getParameter (id))
        {
            const int cur = (int) std::lround (p->convertFrom0to1 (p->getValue()));
            const int next = (cur + delta + count) % count;
            p->setValueNotifyingHost (p->convertTo0to1 ((float) next));
        }
    };

    if (code == 'M')
    {
        setChoice ("scale", scalePads.getSelected() == 0 ? 1 : 0);
        return true;
    }
    if (code == juce::KeyPress::leftKey)  { scalePads.nudge (-1,  0); return true; }
    if (code == juce::KeyPress::rightKey) { scalePads.nudge ( 1,  0); return true; }
    if (code == juce::KeyPress::upKey)    { scalePads.nudge ( 0, -1); return true; }
    if (code == juce::KeyPress::downKey)  { scalePads.nudge ( 0,  1); return true; }
    if (code == 'O') { cycle ("color", 4, 1); return true; }
    if (code == 'I') { cycle ("inversion", 4, 1); return true; }
    if (code == juce::KeyPress::tabKey) { toggle ("layout"); return true; }
    if (code == juce::KeyPress::spaceKey) { toggle ("sustain"); return true; }
    if (code == '[' || code == ']')
    {
        const int cur = (int) anopi::stageFromExtensions (proc.apvts.getRawParameterValue ("extensions")->load());
        const int next = (cur + (code == ']' ? 1 : -1) + 6) % 6;
        static constexpr float kStageValue[] = { 0.09f, 0.30f, 0.50f, 0.65f, 0.79f, 0.93f };
        if (auto* p = proc.apvts.getParameter ("extensions"))
            p->setValueNotifyingHost (kStageValue[next]);
        return true;
    }
    if (code == ';') { step ("voicing", -0.08f); return true; }
    if (code == '\'') { step ("voicing", 0.08f); return true; }
    if (code == juce::KeyPress::F1Key) { toggle ("keysOn"); return true; }
    if (code == juce::KeyPress::F2Key) { toggle ("bassOn"); return true; }
    if (code == juce::KeyPress::F3Key) { toggle ("arpOn"); return true; }
    if (code == juce::KeyPress::F4Key) { toggle ("padOn"); return true; }
    if (code == ',') { bend.setValue (juce::jlimit (0.0, 1.0, bend.getValue() - 0.05)); return true; }
    if (code == '.') { bend.setValue (juce::jlimit (0.0, 1.0, bend.getValue() + 0.05)); return true; }

    syncQwertyDegrees();
    return anopi::ControlMap::qwertyDegree (code).has_value()
        || anopi::ControlMap::qwertyStrum (code).has_value()
        || code == 'B' || code == 'N';
}

juce::AudioProcessorEditor* AnopiAudioProcessor::createEditor()
{
    return new AnopiAudioProcessorEditor (*this);
}
