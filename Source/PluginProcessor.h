/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
//==============================================================================
/**
*/


class Fourier4AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Fourier4AudioProcessor();
    ~Fourier4AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    static constexpr int fftsize = 256;
    float leftbuf[fftsize], rightbuf[fftsize], leftbufout[fftsize], rightbufout[fftsize];
    int playheadL, playheadR, writeheadL = 0, writeheadR = 0;
    dsp::FFT fourierf, fourieri;
    dsp::WindowingFunction<float> window;
    int windowL = 0, windowR = 0;
    void processCorrect(float* bufferinput, float* bufferoutput, int playhead, int writehead);
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fourier4AudioProcessor)
};
