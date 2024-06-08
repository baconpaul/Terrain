#pragma once

#include "Panel.h"
#include "ParameterSlider.h"
namespace ti
{
class Filter : public juce::Component 
{
public:
    Filter (juce::ValueTree controlsBranch, 
            juce::UndoManager& um, 
            GlobalTimer& gt, 
            const tp::Parameters& p)
      : state (controlsBranch), 
        undoManager (um), 
        frequency (p.filterFrequency, gt, "Frequency", {20.0f, 10000.0f}, 500.0f), 
        resonance (p.filterResonance, gt, "Resonance", {0.0f, 1.0f})
    {
        jassert (state.getType() == id::CONTROLS);
        
        label.setText ("Filter", juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
        frequency.getSlider().onValueChange = [&]() {state.setProperty (id::filterFrequency, frequency.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (frequency);
        resonance.getSlider().onValueChange = [&]() {state.setProperty (id::filterResonance, resonance.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (resonance);
    }
    void resized() override 
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (20));
        frequency.setBounds (b.removeFromLeft (100));
        resonance.setBounds (b.removeFromLeft (100));
    }
private:
    juce::ValueTree state;
    juce::UndoManager& undoManager;

    juce::Label label;
    ParameterSlider frequency, resonance;
};
class OverSampling : public juce::Component
{
public:
    OverSampling (juce::ValueTree controlsBranch, 
                  juce::UndoManager& um)
      : state (controlsBranch),
        undoManager (um)
    {
        jassert (state.getType() == id::CONTROLS);
        dropDown.addItem ("1X", 1);
        dropDown.addItem ("2X", 2);
        dropDown.addItem ("4X", 3);
        dropDown.addItem ("8X", 4);
        dropDown.addItem ("16X", 5);
        dropDown.setSelectedId (static_cast<int> (state.getProperty (id::oversampling)) + 1, juce::dontSendNotification);
        dropDown.onChange = [&]() 
            {
                auto index = dropDown.getSelectedItemIndex();
                state.setProperty (id::oversampling, index, &undoManager);
            };
        addAndMakeVisible (dropDown);
        label.setText ("Oversampling", juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    }
    void resized() override 
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (20));
        dropDown.setBounds (b.removeFromTop (20));
    }
private:
    juce::ValueTree state;
    juce::UndoManager& undoManager;

    juce::Label label;
    juce::ComboBox dropDown;
};
class Envelope : public juce::Component
{
public:
    Envelope (juce::ValueTree trajectoryVariablesBranch, 
              juce::UndoManager& um, 
              GlobalTimer& gt, 
              const tp::Parameters& p)
      : state (trajectoryVariablesBranch),
        undoManager (um),
        parameters (p), 
        envelopeSize (parameters.envelopeSize, gt, "ES"),
        attack (parameters.attack, gt, "Attack", {2.0f, 2000.0f}, 100.0f),
        decay (parameters.decay, gt, "Decay", {2.0f, 1000.0f}, 50.0f), 
        sustain (parameters.sustain, gt, "Sustain", {0.0f, 1.0f}), 
        release (parameters.release, gt, "Release", {10.0f, 4000.0f}, 800.0f)
    {
        jassert (state.getType() == id::TRAJECTORY_VARIABLES);
        envelopeSize.getToggle().onStateChange = [&]() { state.setProperty (id::envelopeSize, envelopeSize.getToggle().getToggleState(), &undoManager); };
        addAndMakeVisible (envelopeSize);
        attack.getSlider().onValueChange = [&]() { state.setProperty (id::attack, attack.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (attack);
        decay.getSlider().onValueChange = [&]() { state.setProperty (id::decay, decay.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (decay);
        sustain.getSlider().onValueChange = [&]() { state.setProperty (id::sustain, sustain.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (sustain);
        release.getSlider().onValueChange = [&]() { state.setProperty (id::release, release.getSlider().getValue(), &undoManager); };
        addAndMakeVisible (release);
    }
    void resized() override 
    {
        auto b = getLocalBounds();
        envelopeSize.setBounds (b.removeFromLeft (30));
        attack.setBounds (b.removeFromLeft (100));
        decay.setBounds (b.removeFromLeft (100));
        sustain.setBounds (b.removeFromLeft (100));
        release.setBounds (b.removeFromLeft (100));
    }

private:
    juce::ValueTree state;
    juce::UndoManager& undoManager;
    const tp::Parameters& parameters;
    ti::ParameterToggle envelopeSize;
    ti::ParameterSlider attack, decay, sustain, release;
};
class ControlPanel : public Panel
{
public:
    ControlPanel (juce::ValueTree synthState, 
                  juce::UndoManager& um, 
                  GlobalTimer& gt, 
                  const tp::Parameters& p)
      : Panel ("Control Panel"), 
        state (synthState), 
        undoManager (um), 
        envelope (state.getChildWithName (id::TRAJECTORY_VARIABLES), undoManager, gt, p), 
        oversampling (state.getChildWithName (id::CONTROLS), undoManager), 
        filter (state.getChildWithName (id::CONTROLS), undoManager, gt, p)
    {
        jassert (state.getType() == id::TERRAINSYNTH);
        addAndMakeVisible (envelope);  
        addAndMakeVisible (oversampling);
        addAndMakeVisible (filter);
    }
    void resized() override 
    {
        Panel::resized();
        auto b = getAdjustedBounds();
        envelope.setBounds (b.removeFromLeft (430));
        oversampling.setBounds (b.removeFromLeft (80));
        filter.setBounds (b.removeFromLeft (200));
    }
private:
    juce::ValueTree state;
    juce::UndoManager& undoManager;
    Envelope envelope;
    OverSampling oversampling;
    Filter filter;
};
}