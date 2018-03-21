/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "ARAResamplerProcessor.h"
#include "ARAResamplerEditor.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "ARAUtils/ARAPlugin.h"

// Init static to false
/*static*/ std::atomic_bool ARAResamplerProcessor::s_bInitSamplerStatic(false);

//==============================================================================
ARAResamplerProcessor::ARAResamplerProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor ( BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
					   .withInput  ( "Input", AudioChannelSet::stereo(), true )
#endif
					   .withOutput ( "Output", AudioChannelSet::stereo(), true )
#endif
	)
#endif
	, m_fHostPosInternal( 0 )
{
	m_pEditor = nullptr;

	// THe first instance to see this is false should init static
	if ( s_bInitSamplerStatic.load() == false )
	{
		s_bInitSamplerStatic.store( true );
		UniSampler::InitStatic( 256 );
	}
}

ARAResamplerProcessor::~ARAResamplerProcessor()
{
	m_abARAThreadRun.store( false );
	m_ARAThread.join();
}

UniSampler * ARAResamplerProcessor::GetSampler() const
{
	return m_pSampler.get();
}

//==============================================================================
const String ARAResamplerProcessor::getName() const
{
	return JucePlugin_Name;
}

bool ARAResamplerProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool ARAResamplerProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

double ARAResamplerProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int ARAResamplerProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int ARAResamplerProcessor::getCurrentProgram()
{
	return 0;
}

void ARAResamplerProcessor::setCurrentProgram ( int index )
{
}

const String ARAResamplerProcessor::getProgramName ( int index )
{
	return{};
}

void ARAResamplerProcessor::changeProgramName ( int index, const String& newName )
{
}

//==============================================================================
void ARAResamplerProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	m_pSampler.reset( new UniSampler( sampleRate, 1.f ) );
	m_pSampler->Enable( true );

	// Now that sampler is initialized spawn the ARA thread and load th eplugin
	m_abARAThreadRun.store( true );
	m_ARAThread = std::thread( [this] ()
	{
		// Create plugin instance at start of thread
		// (we're using Melodyne, which I symlink next to the .exe)
		std::unique_ptr<ARAPlugin> pARAPlugin( new ARAPlugin( m_pSampler.get() ) );
		pARAPlugin->LoadPlugin( "Melodyne.vst3" );

		// While this thread needs to run
		while ( m_abARAThreadRun.load() )
		{
			// Check if we have a new sample
			String strSampleToLoad;
			{
				std::lock_guard<std::mutex> lgLoadSample( m_muLoadSample );
				strSampleToLoad = m_strSampleToLoad;
				m_strSampleToLoad.clear();
			}

			// Continue if we don't
			if ( strSampleToLoad.isEmpty() )
				continue;

			// Otherwise ask the plugin to create the synth patch from the sample
			pARAPlugin->CreateSamples( strSampleToLoad.toStdString(), m_pEditor );
		}

		// Destroy ARA plugin
		// (must be destroyed on the thread it was created)
		pARAPlugin.reset();
	} );
}

void ARAResamplerProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ARAResamplerProcessor::isBusesLayoutSupported ( const BusesLayout& layouts ) const
{
#if JucePlugin_IsMidiEffect
	ignoreUnused ( layouts );
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if ( layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
		 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo() )
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if ( layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet() )
		return false;
#endif

	return true;
#endif
}
#endif

void ARAResamplerProcessor::processBlock ( AudioSampleBuffer& buffer, MidiBuffer& midiMessages )
{
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for ( int i = totalNumInputChannels; i < totalNumOutputChannels; ++i )
		buffer.clear( i, 0, buffer.getNumSamples() );

	if ( !dbgASSERT( totalNumOutputChannels == 2, "Invalid output configuration" ) )
		return;

	if ( m_pSampler == nullptr )
		return;

	auto t1 = std::chrono::high_resolution_clock::now();

	AudioPlayHead::CurrentPositionInfo ci{ 0 };
	float fHostPos = 0;
	uint32_t uTempoBPM = 120;
	EPlayState eState = EPlayState::STOPPED;
	if ( getPlayHead() && getPlayHead()->getCurrentPosition( ci ) )
	{
		fHostPos = (ci.timeInSeconds / 60.f) * ci.bpm;
		uTempoBPM = ci.bpm;
		eState = EPlayState::PLAYING; // Starting?
	}
	else
	{
		float fSecPerBuf= buffer.getNumSamples() / (float) m_pSampler->GetSampleRate();
		float fBeatsPerBuf = (fSecPerBuf / 60.f) * uTempoBPM;
		m_fHostPosInternal += fBeatsPerBuf;
		eState = EPlayState::PLAYING;
		fHostPos = m_fHostPosInternal;
	}

	float fBeatPerSamp = uTempoBPM / (60.f * m_pSampler->GetSampleRate());
	float fBeatPerBuffer = buffer.getNumSamples() * fBeatPerSamp;

	MidiBuffer::Iterator itBuf( midiMessages );
	MidiMessage message;
	int sampleNumber;
	while ( itBuf.getNextEvent( message, sampleNumber ) )
	{
		unsigned char status = (message.getRawData()[0]);
		unsigned char note = message.getRawData()[1];
		unsigned char vel = message.getRawData()[2];

		// dbgTRACE(sampleNumber, (int)status, (int)note, (int)vel);
		MidiEventPost mpe;
		mpe.uStatus = (message.getRawData()[0]);
		mpe.uData1 = (message.getRawData()[1]);
		mpe.uData2 = (message.getRawData()[2]);
		m_pSampler->PostMidiEvent( mpe );
	}

	m_pSampler->process( fHostPos, fHostPos + fBeatPerBuffer, uTempoBPM, buffer.getArrayOfWritePointers(), buffer.getNumSamples(), eState );

	auto t2 = std::chrono::high_resolution_clock::now();
	auto dur = (t2 - t1).count();

	// remap dur between green and red
	if ( m_pEditor )
		m_pEditor->SetNeedsRepaint( Colours::green.interpolatedWith( Colours::red, (float) std::min<double>( dur / 2500000., 1. ) ) );
}

//==============================================================================
bool ARAResamplerProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* ARAResamplerProcessor::createEditor()
{
	m_pEditor = new ARAResamplerEditor( *this );
	return m_pEditor;
}

//==============================================================================
void ARAResamplerProcessor::getStateInformation ( MemoryBlock& destData )
{
	// NYI
}

void ARAResamplerProcessor::setStateInformation ( const void* data, int sizeInBytes )
{
	// NYI
}

void ARAResamplerProcessor::HandleCommand( CmdPtr pCMD )
{
	switch ( pCMD->eType )
	{
		// Maybe pass a new sample to be loaded on the ARA thread
		case ICommand::Type::SetARASample:
		{
			std::lock_guard<std::mutex> lgLoadSample( m_muLoadSample );
			m_strSampleToLoad = ((cmdSetARASample *) pCMD.get())->strSampleName;
			break;
		}
	}
}

void ARAResamplerProcessor::OnEditorDestroyed()
{
	m_pEditor = nullptr;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new ARAResamplerProcessor();
}
