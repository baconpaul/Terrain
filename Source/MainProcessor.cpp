#include "MainProcessor.h"
#include "MainEditor.h"

#include "DefaultTreeGenerator.h"

static juce::StringArray getChoices (juce::ValueTree tree)
{
    juce::StringArray sa;
    jassert (tree.getType() == id::TRAJECTORIES ||
             tree.getType() == id::TERRAINS);
    for (int i = 0; i < tree.getNumChildren(); i++)
        sa.add (tree.getChild (i).getProperty (id::type).toString());
    return sa;
}
const juce::String MainProcessor::trajectoryNameFromIndex (int i)
{
    auto trajectories = state.getChildWithName (id::TRAJECTORIES);
    return trajectories.getChild (i).getProperty (id::type).toString();
}
//==============================================================================
MainProcessor::MainProcessor()
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        state (DefaultTree::create())
{

    //======================================Trajectory Parameters
    auto trajectoriesBranch = state.getChildWithName (id::TRAJECTORIES);
    jassert (trajectoriesBranch.getType() == id::TRAJECTORIES);
    addParameter (parameters.currentTrajectory = new tp::ChoiceParameter ("Current Trajectory", 
        getChoices (state.getChildWithName (id::TRAJECTORIES)), 
        "", 
        getCurrentTrajectoryIndexFromString (trajectoriesBranch.getProperty (id::currentTrajectory).toString())));

    auto modifiersBranch = getCurrentTrajectoryBranch (trajectoriesBranch).getChildWithName (id::MODIFIERS);
    jassert (modifiersBranch.getType() == id::MODIFIERS);
    addParameter (parameters.trajectoryModA = new tp::NormalizedFloatParameter ("Trajectory Mod A", modifiersBranch.getProperty (id::mod_A)));
    addParameter (parameters.trajectoryModB = new tp::NormalizedFloatParameter ("Trajectory Mod B", modifiersBranch.getProperty (id::mod_B)));
    addParameter (parameters.trajectoryModC = new tp::NormalizedFloatParameter ("Trajectory Mod C", modifiersBranch.getProperty (id::mod_C)));
    addParameter (parameters.trajectoryModD = new tp::NormalizedFloatParameter ("Trajectory Mod D", modifiersBranch.getProperty (id::mod_D)));
    
    auto trajectoryVariablesBranch = state.getChildWithName (id::TRAJECTORY_VARIABLES);
    jassert (trajectoryVariablesBranch.getType() == id::TRAJECTORY_VARIABLES);
    addParameter (parameters.trajectorySize = new tp::NormalizedFloatParameter ("Size", trajectoryVariablesBranch.getProperty (id::size)));
    addParameter (parameters.trajectoryRotation = new tp::RangedFloatParameter ("Rotation", 
                                                                                {0.0f, juce::MathConstants<float>::twoPi}, 
                                                                                trajectoryVariablesBranch.getProperty (id::rotation)));
    addParameter (parameters.trajectoryTranslationX = new tp::RangedFloatParameter ("Translation X", 
                                                                                    {-1.0f, 1.0f}, 
                                                                                    (trajectoryVariablesBranch.getProperty (id::translation_x))));
    addParameter (parameters.trajectoryTranslationY = new tp::RangedFloatParameter ("Translation Y", 
                                                                                    {-1.0f, 1.0f},
                                                                                    (trajectoryVariablesBranch.getProperty (id::translation_y))));
    addParameter (parameters.envelopeSize = new juce::AudioParameterBool ("envelopeSize", "Envelope Size", true));
    auto range = juce::NormalisableRange<float> (2.0f, 2000.0f); range.setSkewForCentre (100.0f);
    addParameter (parameters.attack = new tp::RangedFloatParameter ("Attack", 
                                                                    range, 
                                                                    trajectoryVariablesBranch.getProperty (id::attack)));
    range = juce::NormalisableRange<float> (2.0f, 1000.0f); range.setSkewForCentre (50.0f);
    addParameter (parameters.decay = new tp::RangedFloatParameter ("Decay", 
                                                                    range, 
                                                                    trajectoryVariablesBranch.getProperty (id::decay)));
    addParameter (parameters.sustain = new tp::NormalizedFloatParameter ("sustain", trajectoryVariablesBranch.getProperty (id::sustain)));
    range = juce::NormalisableRange<float> (10.0f, 4000.0f); range.setSkewForCentre (800.0f);
    addParameter (parameters.release = new tp::RangedFloatParameter ("Release", 
                                                                    range, 
                                                                    trajectoryVariablesBranch.getProperty (id::release)));
    
    
    auto trajectoryFeedbackBranch = trajectoryVariablesBranch.getChildWithName (id::FEEDBACK);
    jassert (trajectoryFeedbackBranch.getType() == id::FEEDBACK);
    range = juce::NormalisableRange<float> (0.0f, 2000.0f); range.setSkewForCentre (250.0f);
    addParameter (parameters.feedbackTime = new tp::RangedFloatParameter ("Feedback Time", 
                                                                          range,
                                                                          (trajectoryFeedbackBranch.getProperty (id::feedbackTime))));
    range = juce::NormalisableRange<float> (0.0f, 0.9999f); range.setSkewForCentre (0.8f);
    addParameter (parameters.feedbackScalar = new tp::RangedFloatParameter ("Feedback", 
                                                                            range,
                                                                            (trajectoryVariablesBranch.getProperty (id::feedbackScalar))));
    addParameter (parameters.feedbackCompression = new tp::RangedFloatParameter ("Feedback Compression", 
                                                                                 {1.0f, 20.0f},
                                                                                 (trajectoryVariablesBranch.getProperty (id::feedbackCompression))));
    addParameter (parameters.feedbackMix = new tp::RangedFloatParameter ("Feedback Mix", 
                                                                         {0.0f, 1.0f},
                                                                         (trajectoryVariablesBranch.getProperty (id::feedbackScalar))));

    //=======================================Terrain Parameters
    auto terrainsBranch = state.getChildWithName (id::TERRAINS);
    jassert (terrainsBranch.getType() == id::TERRAINS);
    addParameter (parameters.currentTerrain = new tp::ChoiceParameter ("Current Terrain", 
        getChoices (state.getChildWithName (id::TERRAINS)), 
        "",  
        getCurrentTerrainIndexFromString (terrainsBranch.getProperty (id::currentTerrain).toString())));
    modifiersBranch = terrainsBranch.getChildWithName (id::MODIFIERS);
    addParameter (parameters.terrainModA = new tp::NormalizedFloatParameter ("Terrain Mod A", modifiersBranch.getProperty (id::mod_A)));
    addParameter (parameters.terrainModB = new tp::NormalizedFloatParameter ("Terrain Mod B", modifiersBranch.getProperty (id::mod_B)));
    addParameter (parameters.terrainModC = new tp::NormalizedFloatParameter ("Terrain Mod C", modifiersBranch.getProperty (id::mod_C)));
    addParameter (parameters.terrainModD = new tp::NormalizedFloatParameter ("Terrain Mod D", modifiersBranch.getProperty (id::mod_D)));

    state.addListener (this);

    synthesizer = std::make_unique<tp::WaveTerrainSynthesizer> (parameters);
}

MainProcessor::~MainProcessor() {}
//==============================================================================
const juce::String MainProcessor::getName() const  { return JucePlugin_Name; }
bool MainProcessor::acceptsMidi() const            { return true; }
bool MainProcessor::producesMidi() const           { return false; }
bool MainProcessor::isMidiEffect() const           { return false; }
double MainProcessor::getTailLengthSeconds() const { return 0.0; }
int MainProcessor::getNumPrograms() { return 1; }

int MainProcessor::getCurrentProgram()             { return 0;  }
void MainProcessor::setCurrentProgram (int index)  { juce::ignoreUnused (index); }
const juce::String MainProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}
void MainProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }
//==============================================================================
void MainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) 
{ 
    synthesizer->prepareToPlay (sampleRate, samplesPerBlock);
}
void MainProcessor::releaseResources() {}
bool MainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
void MainProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    synthesizer->updateTerrain();
    synthesizer->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}
//==============================================================================
bool MainProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* MainProcessor::createEditor() { return new MainEditor (*this); }
//==============================================================================
void MainProcessor::getStateInformation (juce::MemoryBlock& destData) 
{ 
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}
void MainProcessor::setStateInformation (const void* data, int sizeInBytes)
{ 
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() == nullptr) return; // make sure we have data
    if (!xml->hasTagName (state.getType())) return; // make sure it's the right data

    state = juce::ValueTree::fromXml (*xml);
}
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MainProcessor(); }