/*
==============================================================================

This file was auto-generated!

It contains the basic framework code for a JUCE plugin editor.

==============================================================================
*/

#include "ARAResamplerProcessor.h"
#include "ARAResamplerEditor.h"

#include "../UniSampler/Source/Plugin/UniSampler.h"

//==============================================================================
ARAResamplerEditor::ARAResamplerEditor( ARAResamplerProcessor& p )
	: AudioProcessorEditor( &p ), processor( p ),
	m_Resizer( this, &m_CBC )
	, keyboardComponent( midiState, MidiKeyboardComponent::horizontalKeyboard )
	, m_aiKeysLoaded( 0 )
	, m_AudioImageRenderer( this )
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	setSize ( 700, 700 );

	// Add audio renderer, set initial color to red
	addAndMakeVisible( m_AudioImageRenderer );
	m_AudioImageRenderer.SetColor( Colours::red );

	// MIDI keyboard
	addAndMakeVisible( keyboardComponent );
	setWantsKeyboardFocus( true );
	
	// Resizer component
	addAndMakeVisible( m_Resizer );
	m_CBC.setSizeLimits( 150, 150, 1280, 800 );

	// Flags and timers
	m_abNeedsRepaint = false;
	m_EditorColor = getLookAndFeel().findColour( ResizableWindow::backgroundColourId );
	startTimer( (int) ETimerID::REPAINT, 40 );
	startTimer( (int) ETimerID::KBD_FOCUS, 400 );
}

// If we have a new wave file to load, send it over
// to the ARA thread as a command instance
void ARAResamplerEditor::OnNewWave( File& f )
{
	cmdSetARASample cmd;
	cmd.strSampleName = f.getFullPathName();
	processor.HandleCommand( CmdPtr( new cmdSetARASample( cmd ) ) );
}

// Used to update the UI as the ARA thread processes a sample
void ARAResamplerEditor::HandleCommand( CmdPtr pCMD )
{
	switch ( pCMD->eType )
	{
		// When the sample is loaded draw the waveform as red
		case ICommand::Type::SampleLoaded:
			m_AudioImageRenderer.SetWave( ((cmdSampleLoaded *) pCMD.get())->pSample );
			m_AudioImageRenderer.SetColor( Colours::red );
			break;
		// Each time a region is added increase the number of visible keys
		// (assume these come in order)
		case ICommand::Type::RegionCreated:
			m_aiKeysLoaded.store( ((cmdRegionCreated *) pCMD.get())->iMIDINote );
			break;
		// Once the program is set, draw the waveform green
		case ICommand::Type::ProgramSet:
			m_AudioImageRenderer.SetColor( Colours::green );
			break;
		default:
			break;
	}
}

ARAResamplerEditor::~ARAResamplerEditor()
{
	processor.OnEditorDestroyed();
}

void ARAResamplerEditor::SetNeedsRepaint( Colour c )
{
	m_abNeedsRepaint = true;
	m_EditorColor = c;
}

void ARAResamplerEditor::timerCallback(int iTimerID)
{
	int ixKeyMax = -1;
	switch ( (ETimerID) iTimerID )
	{
		case ETimerID::REPAINT:
			// Use this timer to see if we need to add more MIDI keys
			ixKeyMax = m_aiKeysLoaded.exchange( -1 );
			if ( ixKeyMax >= 0 )
				keyboardComponent.setAvailableRange( 0, ixKeyMax );

			// Maybe repaint (relatively unused)
			if ( m_abNeedsRepaint )
			{
				m_abNeedsRepaint = false;
				repaint();
			}
			break;
			// Keyboard focus timer - cancel immediately
		case ETimerID::KBD_FOCUS:
			keyboardComponent.hasKeyboardFocus( true );
			stopTimer( iTimerID );
			break;
	}
}

//==============================================================================
void ARAResamplerEditor::paint ( Graphics& g )
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll( m_EditorColor );

	g.setColour ( Colours::white );
	g.setFont ( 15.0f );

}

void ARAResamplerEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
	juce::Rectangle<int> rcBounds = getBounds();
	m_Resizer.setBounds( rcBounds.getWidth() - 16, rcBounds.getHeight() - 16, 16, 16 );

	// Use most of the space for the audio image renderer
	int nAudioGLBottom = .8 * rcBounds.getHeight();
	m_AudioImageRenderer.setBounds( 0, 0, rcBounds.getWidth(), nAudioGLBottom );

	// Construct keyboard relative to the audio renderer
	auto imgBounds = m_AudioImageRenderer.getBounds();
	imgBounds.translate( 0, imgBounds.getHeight() );
	imgBounds.setHeight( getHeight() - imgBounds.getHeight() );
	keyboardComponent.setBounds( imgBounds );

	// No keys to start
	keyboardComponent.setAvailableRange( 0, 0 );
}