/*
  ==============================================================================

    SpectrumAnalyzerComponent.cpp
    Created: 22 Apr 2021 1:10:11pm
    Author:  David López Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrumAnalyzerComponent.h"

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent() : leftPathProducer(leftChannelFifo), rightPathProducer(rightChannelFifo),
														 forwardFFT(fftOrder), spectrogramImage(juce::Image::RGB, 512, 512, true)
{
	setOpaque(true);
	startTimerHz(30);//30
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
	stopTimer();
}

void SpectrumAnalyzerComponent::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if (buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	auto responseArea = getAnalysisArea();
	auto renderArea = getRenderArea();

	g.setOpacity(1.0f);

	if (isRMS)
	{
		repaint();
		g.drawImage(backgroundRMS, getLocalBounds().toFloat());

		auto leftChannelFFTPath = leftPathProducer.getPath();
		auto rightChannelFFTPath = rightPathProducer.getPath();

		leftChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseArea.getX(), -10.0f));
		rightChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseArea.getX(), -10.0f));

		g.setColour(juce::Colours::white);
		g.strokePath(leftChannelFFTPath, juce::PathStrokeType(1.0f));

		g.setColour(juce::Colours::skyblue);
		g.strokePath(rightChannelFFTPath, juce::PathStrokeType(1.0f));

		g.setColour(juce::Colours::orange);
		g.drawRoundedRectangle(responseArea.toFloat(), 4.0f, 1.0f);

		g.clipRegionIntersects(getLocalBounds());
	}
	else
	{
		repaint();
		g.drawImage(spectrogramImage, getAnalysisArea().toFloat());
	}
}



void SpectrumAnalyzerComponent::resized()
{
	backgroundRMS = juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true);

	juce::Graphics g(backgroundRMS);

	juce::Array<float> freq
	{
		20, 50, 100,
		200, 500, 1000,
		2000, 5000, 10000, 20000
	};

	auto renderArea = getAnalysisArea();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getHeight();
	auto width = renderArea.getWidth();

	//lines representation
	juce::Array<float> xPos;
	for(auto f : freq)
	{
		auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
		xPos.add(left + width * normX);
	}

	g.setColour(juce::Colours::dimgrey);
	for(auto x : xPos)
	{
		g.drawVerticalLine(x, top, bottom);
	}

	juce::Array<float> gain
	{
		-24, -12, 0, 12, 24
	};
	
	for(auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottom), float(top));
		g.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::darkgrey);
		g.drawHorizontalLine(y, left, right); 
	}

	//Values representation
	g.setColour(juce::Colours::lightgrey);
	const int fontHeight = 10;
	g.setFont(fontHeight);

	for(int i = 0; i < freq.size(); ++i)
	{
		auto f = freq[i];
		auto x = xPos[i];

		bool addK = false;
		juce::String str;
		if(f > 999.0f)
		{
			addK = true;
			f /= 1000.0f;
		}

		str << f;
		if (addK)
			str << "k";
		str << "Hz";

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	for(auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottom), float(top));

		juce::String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth);
		r.setCentre(r.getCentreX(), y);

		g.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::lightgrey);

		g.drawFittedText(str, r, juce::Justification::centred, 1);

		str.clear();
		str << (gDb - 24.0f);

		r.setX(1);
		textWidth = g.getCurrentFont().getStringWidth(str);
		r.setSize(textWidth, fontHeight);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

}

void SpectrumAnalyzerComponent::selGrid(const int choice)
{
	switch(choice)
	{
	case 0:
		isRMS = true;
		break;
	case 1:
		isRMS = false;
		break;
	default:
		jassertfalse;
		break;
	}
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getRenderArea()
{
	auto area = getLocalBounds();
	
	if (isRMS)
	{
		area.removeFromTop(15);
		area.removeFromBottom(0);
		area.removeFromRight(20);
		area.removeFromLeft(20);
	}
	else
	{
		area.removeFromTop(0);
		area.removeFromBottom(-3);
		area.removeFromRight(31);
		area.removeFromLeft(0);
	}

	return area;
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getAnalysisArea()
{
	auto area = getRenderArea();
	
	if (isRMS)
	{
		area.removeFromTop(0);
		area.removeFromBottom(2);
	}
	else
	{
		area.removeFromTop(4);
		area.removeFromBottom(4);//4
	}
	return area;
}

void SpectrumAnalyzerComponent::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
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

void SpectrumAnalyzerComponent::timerCallback()
{
	if (isRMS)
	{
		auto fftBounds = getAnalysisArea().toFloat();

		leftPathProducer.process(fftBounds, sampleRate);
		rightPathProducer.process(fftBounds, sampleRate);

		repaint();
	}
	else
	{
		if (nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			nextFFTBlockReady = false;
			repaint();
		}
	}
}

void SpectrumAnalyzerComponent::pushNextSampleIntoFifo(float sample) noexcept
{
	if (fifoIndex == fftSize)
	{
		if (!nextFFTBlockReady)
		{
			juce::zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));
			nextFFTBlockReady = true;
		}
		fifoIndex = 0;
	}
	fifo[fifoIndex++] = sample;
}

void SpectrumAnalyzerComponent::drawNextLineOfSpectrogram()
{
	const int rightHandEdge = spectrogramImage.getWidth() - 1;
	const int imageHeight = spectrogramImage.getHeight();

	spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	juce::Range<float> maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);
	if (maxLevel.getEnd() == 0.0f)
		maxLevel.setEnd(4.1f);//0.00001

	for (int i = 1; i < imageHeight; ++i)
	{
		const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * 0.4f);//0.2f
		const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
		const float level = juce::jmap(fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, 3.9f);//Original targetRangeMax = 1.0f, needs to be tweaked/tested

		spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSL(level, 1.0f, level, 1.0f));//Colour::fromHSV
	}

}

