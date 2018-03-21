/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "ARAUtils/ARAPlugin.h"

/*static*/ std::atomic_bool JuceSamplerProcessor::s_bInitStaticFX(false);

//==============================================================================
JuceSamplerProcessor::JuceSamplerProcessor()
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

	if ( s_bInitStaticFX.load() == false )
	{
		s_bInitStaticFX.store( true );
		UniSampler::InitStatic( 256 );
	}

	enum class ARAState
	{
		Init,
		LoadSample,

	};
}

JuceSamplerProcessor::~JuceSamplerProcessor()
{
	m_abARAThreadRun.store( false );
	m_ARAThread.join();
}

UniSampler * JuceSamplerProcessor::GetSampler() const
{
	return m_pSampler.get();
}

//==============================================================================
const String JuceSamplerProcessor::getName() const
{
	return JucePlugin_Name;
}

bool JuceSamplerProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool JuceSamplerProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

double JuceSamplerProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int JuceSamplerProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int JuceSamplerProcessor::getCurrentProgram()
{
	return 0;
}

void JuceSamplerProcessor::setCurrentProgram ( int index )
{
}

const String JuceSamplerProcessor::getProgramName ( int index )
{
	return{};
}

void JuceSamplerProcessor::changeProgramName ( int index, const String& newName )
{
}

//==============================================================================
void JuceSamplerProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	m_pSampler.reset( new UniSampler( sampleRate, 1.f ) );
	m_pSampler->Enable( true );


	// m_pARAPlugin->LoadPlugin( "Melodyne.vst3" );
	m_abARAThreadRun.store( true );
	m_ARAThread = std::thread( [this] ()
	{
		// Create plugin instance at start of thread
		std::unique_ptr<ARAPlugin> pARAPlugin( new ARAPlugin( m_pSampler.get() ) );
		pARAPlugin->LoadPlugin( "Melodyne.vst3" );

		while ( m_abARAThreadRun.load() )
		{
			String strSampleToLoad;
			{
				std::lock_guard<std::mutex> lgLoadSample( m_muLoadSample );
				strSampleToLoad = m_strSampleToLoad;
				m_strSampleToLoad.clear();
			}
			if ( strSampleToLoad.isEmpty() )
				continue;

			pARAPlugin->CreateSamples( strSampleToLoad.toStdString(), m_pEditor );
		}

		// Must be destroyed on the thread it was created
		pARAPlugin.reset();
	} );
}

void JuceSamplerProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JuceSamplerProcessor::isBusesLayoutSupported ( const BusesLayout& layouts ) const
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

void JuceSamplerProcessor::processBlock ( AudioSampleBuffer& buffer, MidiBuffer& midiMessages )
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
bool JuceSamplerProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* JuceSamplerProcessor::createEditor()
{
	m_pEditor = new JuceSamplerEditor( *this );
	return m_pEditor;
}

//==============================================================================
void JuceSamplerProcessor::getStateInformation ( MemoryBlock& destData )
{
}

void JuceSamplerProcessor::setStateInformation ( const void* data, int sizeInBytes )
{
}

void JuceSamplerProcessor::HandleCommand( CmdPtr pCMD )
{
	switch ( pCMD->eType )
	{
		case ICommand::Type::SetARASample:
		{
			std::lock_guard<std::mutex> lgLoadSample( m_muLoadSample );
			m_strSampleToLoad = ((cmdSetARASample *) pCMD.get())->strSampleName;
			break;
		}
	}
}

bool JuceSamplerProcessor::SetRootPathString( String strDefaultPathString )
{
	std::string strDefaultPathStringSTD = strDefaultPathString.toStdString();

	m_mbRootPathText.reset();
	m_mbRootPathText.ensureSize( strDefaultPathStringSTD.length() + 1 );
	m_mbRootPathText.copyFrom( strDefaultPathStringSTD.data(), 0, strDefaultPathStringSTD.length() + 1 );
	return true;
}

String JuceSamplerProcessor::GetCurrentFile()
{
	return m_mbProgramText.toString();
}

String JuceSamplerProcessor::GetRootPath()
{
	return m_mbRootPathText.toString();
}

void JuceSamplerProcessor::OnEditorDestroyed()
{
	m_pEditor = nullptr;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new JuceSamplerProcessor();
}
