/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>
#include <algorithm>
#include <iterator>

//==============================================================================
Fourier4AudioProcessor::Fourier4AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), fourierf(log2(fftsize)), fourieri(log2(fftsize)), window(fftsize, dsp::WindowingFunction<float>::hann, false, 0.0f)
#endif
{
    playheadL = 0;
    playheadR = 0;
    std::fill(leftbuf, leftbuf + (fftsize), 0.0f);
    std::fill(rightbuf, rightbuf + (fftsize), 0.0f);
    std::fill(leftbufout, leftbufout + (fftsize), 0.0f);
    std::fill(rightbufout, rightbufout + (fftsize), 0.0f);
}

Fourier4AudioProcessor::~Fourier4AudioProcessor()
{
}

//==============================================================================
const juce::String Fourier4AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Fourier4AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Fourier4AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Fourier4AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Fourier4AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Fourier4AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Fourier4AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Fourier4AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Fourier4AudioProcessor::getProgramName (int index)
{
    return {};
}

void Fourier4AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Fourier4AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void Fourier4AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Fourier4AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Fourier4AudioProcessor::processCorrect(float* bufferinput, float* bufferoutput, int playhead, int writehead){
    float segmentedL[(fftsize * 2)];
    float magL[fftsize];
    float phaseL[fftsize];
    std::fill(segmentedL, segmentedL + (fftsize * 2), 0.0f);

    int counter = 0;
    for (int i = playhead; i < fftsize; i++) {
        segmentedL[counter] = bufferinput[i];
        counter++;
    }

    for (int i = 0; i < playhead; i++) {
        segmentedL[counter] = bufferinput[i];
        counter++;
    }

    window.multiplyWithWindowingTable(segmentedL, fftsize);
    fourierf.performRealOnlyForwardTransform(segmentedL);
    for (int i = 0; i < fftsize; i++) {
        std::complex<float> temp;
        temp.real(segmentedL[i * 2]);
        temp.imag(segmentedL[i * 2 + 1]);

        magL[i] = std::abs(temp);
        phaseL[i] = std::arg(temp);
    }

    

    //custom processing

    //std::reverse(magL, magL + (fftsize / 2));
    //std::reverse(phaseL, phaseL + (fftsize / 2));
    //end processing

    // convert back to real imaginary
    for (int i = 0; i < fftsize; i++) {
        float real = std::cos(phaseL[i]) * magL[i];
        float imag = std::sin(phaseL[i]) * magL[i];

        segmentedL[i * 2] = real;
        segmentedL[i * 2 + 1] = imag;
    }
    fourieri.performRealOnlyInverseTransform(segmentedL);
    window.multiplyWithWindowingTable(segmentedL, fftsize);


    for (int i = 0; i < fftsize; i++) {
        if (i < (fftsize / 4) * 3) { // 75% overlap
            bufferoutput[(writehead + i) % fftsize] += segmentedL[i] * (2.f / 3.f);
        }
        else {
            bufferoutput[(writehead + i) % fftsize] = segmentedL[i] * (2.f / 3.f);
        }
    }
}

void Fourier4AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        if(channel == 0){
            //left
            for(int x = 0; x < buffer.getNumSamples(); x++){
                float f = channelData[x];
                
                //channelData[x] = leftbuf[playheadL];
                channelData[x] = leftbufout[writeheadL];
                leftbuf[playheadL] = f;
                writeheadL++;
                playheadL++;
                windowL++;
                if (windowL == fftsize / 4) {
                    windowL = 0;
                    processCorrect(leftbuf, leftbufout, playheadL, writeheadL);
                }

                if(writeheadL == fftsize){
                    writeheadL = 0;
                }
                if(playheadL == fftsize){
                    playheadL = 0;
                }
                
                
            }
        }else{
            for (int x = 0; x < buffer.getNumSamples(); x++) {
                float f = channelData[x];

                channelData[x] = rightbufout[writeheadR];
                rightbuf[playheadR] = f;

                playheadR++;
                writeheadR++;
                windowR++;
                if(windowR == fftsize / 4){
                    windowR = 0;
                    processCorrect(rightbuf, rightbufout, playheadR, writeheadR);
                }

                if (playheadR == fftsize) {
                    playheadR = 0;
                    
                    
                }

                if(writeheadR == fftsize){
                    writeheadR = 0;
                }


            }
        }
        // ..do something to the data...
    }
}

//==============================================================================
bool Fourier4AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Fourier4AudioProcessor::createEditor()
{
    return new Fourier4AudioProcessorEditor (*this);
}

//==============================================================================
void Fourier4AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Fourier4AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Fourier4AudioProcessor();
}
