#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VibeSparkProcessor::VibeSparkProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VibeSparkProcessor::~VibeSparkProcessor()
{
}

//==============================================================================
const juce::String VibeSparkProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VibeSparkProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VibeSparkProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VibeSparkProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VibeSparkProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VibeSparkProcessor::getNumPrograms()
{
    return 1;
}

int VibeSparkProcessor::getCurrentProgram()
{
    return 0;
}

void VibeSparkProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VibeSparkProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VibeSparkProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void VibeSparkProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void VibeSparkProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VibeSparkProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VibeSparkProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::ignoreUnused (midiMessages);
}

//==============================================================================
bool VibeSparkProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VibeSparkProcessor::createEditor()
{
    return new VibeSparkAudioProcessorEditor (*this);
}

//==============================================================================
void VibeSparkProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void VibeSparkProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

void VibeSparkProcessor::cutAll(int channel)
{
    juce::ignoreUnused (channel);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeSparkProcessor();
}
