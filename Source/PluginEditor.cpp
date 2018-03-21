/*
==============================================================================

This file was auto-generated!

It contains the basic framework code for a JUCE plugin editor.

==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "../UniSampler/Source/Plugin/UniSampler.h"

//==============================================================================
JuceSamplerEditor::JuceSamplerEditor( JuceSamplerProcessor& p )
	: AudioProcessorEditor( &p ), processor( p ),
	m_Resizer( this, &m_CBC )
	, keyboardComponent( midiState, MidiKeyboardComponent::horizontalKeyboard )
	, m_aiKeysLoaded( 0 )
	, m_AudioImageRenderer( this )
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	setSize ( 700, 700 );

	addAndMakeVisible( m_AudioImageRenderer );
	m_AudioImageRenderer.SetColor( Colours::red );

	addAndMakeVisible( keyboardComponent );
	setWantsKeyboardFocus( true );

	addAndMakeVisible( m_Resizer );
	m_CBC.setSizeLimits( 150, 150, 1280, 800 );

	m_abNeedsRepaint = false;
	m_EditorColor = getLookAndFeel().findColour( ResizableWindow::backgroundColourId );
	startTimer( (int) ETimerID::REPAINT, 40 );
	startTimer( (int) ETimerID::KBD_FOCUS, 400 );
}

void JuceSamplerEditor::OnNewWave( File& f )
{
	cmdSetARASample cmd;
	cmd.strSampleName = f.getFullPathName();
	processor.HandleCommand( CmdPtr( new cmdSetARASample( cmd ) ) );
}

void JuceSamplerEditor::HandleCommand( CmdPtr pCMD )
{
	switch ( pCMD->eType )
	{
		case ICommand::Type::SampleLoaded:
			m_AudioImageRenderer.SetWave( ((cmdSampleLoaded *) pCMD.get())->pSample );
			m_AudioImageRenderer.SetColor( Colours::red );
			break;
		case ICommand::Type::RegionCreated:
			// Assume these come in MIDI note order
			m_aiKeysLoaded.store( ((cmdRegionCreated *) pCMD.get())->iMIDINote );
			break;
		case ICommand::Type::ProgramSet:
			m_AudioImageRenderer.SetColor( Colours::green );
			break;
		case ICommand::Type::SetARASample:
		default:
			break;
	}
}

JuceSamplerEditor::~JuceSamplerEditor()
{
	processor.OnEditorDestroyed();
}

void JuceSamplerEditor::SetNeedsRepaint( Colour c )
{
	m_abNeedsRepaint = true;
	m_EditorColor = c;
}

void JuceSamplerEditor::timerCallback(int iTimerID)
{
	int ixKeyMax = -1;
	switch ( (ETimerID) iTimerID )
	{
		case ETimerID::REPAINT:

			ixKeyMax = m_aiKeysLoaded.exchange( -1 );
			if ( ixKeyMax >= 0 )
				keyboardComponent.setAvailableRange( 0, ixKeyMax );

			if ( m_abNeedsRepaint )
			{
				m_abNeedsRepaint = false;
				repaint();
			}
			break;
		case ETimerID::KBD_FOCUS:
			keyboardComponent.hasKeyboardFocus( true );
			stopTimer( iTimerID );
			break;
		case ETimerID::KBD_LOAD:
			int iKeysLoaded = m_aiKeysLoaded.fetch_add( 1 );
			keyboardComponent.setAvailableRange( 0, iKeysLoaded);
			if ( iKeysLoaded >= 127 )
				stopTimer( iTimerID );
			break;
	}
}

//==============================================================================
void JuceSamplerEditor::paint ( Graphics& g )
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll( m_EditorColor );

	g.setColour ( Colours::white );
	g.setFont ( 15.0f );
	//g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);

}

void JuceSamplerEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
	juce::Rectangle<int> rcBounds = getBounds();
	m_Resizer.setBounds( rcBounds.getWidth() - 16, rcBounds.getHeight() - 16, 16, 16 );

	// 90% of height is text editor
	int nSFZEditBottom = .8 * rcBounds.getHeight();
	m_AudioImageRenderer.setBounds( 0, 0, rcBounds.getWidth(), nSFZEditBottom );

	auto imgBounds = m_AudioImageRenderer.getBounds();

	imgBounds.translate( 0, imgBounds.getHeight() );
	imgBounds.setHeight( getHeight() - imgBounds.getHeight() );
	keyboardComponent.setBounds( imgBounds );
	keyboardComponent.setAvailableRange( 0, 0 );
}