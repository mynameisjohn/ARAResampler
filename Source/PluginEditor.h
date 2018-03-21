/*
==============================================================================

This file was auto-generated!

It contains the basic framework code for a JUCE plugin editor.

==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

#include <map>
#include <atomic>
#include <stdint.h>

#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "AudioImageGL.h"
#include "ICommand.h"

//==============================================================================
/**
*/
class JuceSamplerEditor : public AudioProcessorEditor,
	private MultiTimer,
	public ICommandListener
{
	enum class ETimerID
	{
		REPAINT,
		KBD_FOCUS,
		KBD_LOAD
	};
	void timerCallback( int iTimerID ) override;
	std::atomic_bool m_abNeedsRepaint;
	Colour m_EditorColor;

public:
	void SetNeedsRepaint( Colour newColor );

	JuceSamplerEditor ( JuceSamplerProcessor& );
	~JuceSamplerEditor();

	//==============================================================================
	void paint ( Graphics& ) override;
	void resized() override;

	void HandleCommand( CmdPtr pCMD ) override;

	void OnNewWave( File& f );

private:
	// Audio thumbnail
	AudioImageGL m_AudioImageRenderer;

	// MIDI keyboard
	std::atomic<int> m_aiKeysLoaded;
	MidiKeyboardState midiState;
	MidiKeyboardComponent keyboardComponent;

	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	JuceSamplerProcessor& processor;

	// Window is resizable
	ResizableCornerComponent m_Resizer;
	ComponentBoundsConstrainer m_CBC;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( JuceSamplerEditor )
};
