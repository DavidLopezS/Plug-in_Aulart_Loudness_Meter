/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_MeterAudioProcessorEditor::Loudness_MeterAudioProcessorEditor (Loudness_MeterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), gridRepresentation(p), mydBKnobs(juce::Colours::darkgrey)
{

	juce::StringArray choices{ "RMS", "Spectrogram" };
	spectrRMSSelector.addItemList(choices, 1);
	addAndMakeVisible(&spectrRMSSelector);

	spectrRMSSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "GRAFTYPE", spectrRMSSelector);

	addAndMakeVisible(&gridRepresentation);

	addAndMakeVisible(&mydBKnobs);

	for (int i = 0; i < numKnobs; ++i)
		knobAttachment(i);

    setSize (900, 500);
}

Loudness_MeterAudioProcessorEditor::~Loudness_MeterAudioProcessorEditor()
{
	myAttachments.clear(); 
}

void Loudness_MeterAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey);
}

void Loudness_MeterAudioProcessorEditor::resized()
{
	juce::Grid grid;

	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = { Track(Fr(2)), Track(Fr(1)), Track(Fr(1)) };
	grid.templateColumns = { Track(Fr(1)) };

	grid.items = { juce::GridItem(gridRepresentation), juce::GridItem(spectrRMSSelector), juce::GridItem(mydBKnobs) };

	grid.performLayout(getLocalBounds());
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
	juce::AudioBuffer<float> tempIncomingBuffer;
	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size);

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
		}
	}

	const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
	const auto binWidth = sampleRate / double(fftSize);

	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftData;
		if (leftChannelFFTDataGenerator.getFFTData(fftData))
		{
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
		}
	}

	while (pathProducer.getNumPathsAvailable() > 0)
	{
		pathProducer.getPath(leftChannelFFTPath);
	}
}

void Loudness_MeterAudioProcessorEditor::knobAttachment(int knobId)
{
	auto &myKnobs = *mydBKnobs.myKnobs[knobId];

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	myAttachments.push_back(std::make_unique<Attachment>(audioProcessor.apvts, myKnobName[knobId], myKnobs));
}
