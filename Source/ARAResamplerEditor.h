/******************************************************************************

ARAResamplerEditor.h

Based on the basic JUCE plugin editor. Our Editor window has an OpenGL
component where we draw our loaded sample waveform (if one exists) and 
a MidiKeyboard component that indicates how many samples have been loaded

******************************************************************************/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ARAResamplerProcessor.h"

#include <map>
#include <atomic>
#include <stdint.h>

#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "AudioImageGL.h"
#include "ICommand.h"

//==============================================================================
/**
*/
class ARAResamplerEditor : public AudioProcessorEditor,
	private MultiTimer,
	public ICommandListener
{
	// Timer IDs
	enum class ETimerID
	{
		REPAINT,
		KBD_FOCUS
	};
	void timerCallback( int iTimerID ) override;

	// Editor color and repaint flag
	std::atomic_bool m_abNeedsRepaint;
	Colour m_EditorColor;

public:

	ARAResamplerEditor ( ARAResamplerProcessor& );
	~ARAResamplerEditor();

	//==============================================================================
	void paint ( Graphics& ) override;
	void resized() override;

	// Receives info from the ARA thread on load status
	void HandleCommand( CmdPtr pCMD ) override;

	// Tell the ARA thread to load a new sample
	void OnNewWave( File& f );

	void SetNeedsRepaint( Colour newColor );

private:
	// Audio waveform drawer
	AudioImageGL m_AudioImageRenderer;

	// MIDI keyboard
	std::atomic<int> m_aiKeysLoaded;
	MidiKeyboardState midiState;
	MidiKeyboardComponent keyboardComponent;

	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	ARAResamplerProcessor& processor;

	// Window is resizable
	ResizableCornerComponent m_Resizer;
	ComponentBoundsConstrainer m_CBC;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( ARAResamplerEditor )
};
