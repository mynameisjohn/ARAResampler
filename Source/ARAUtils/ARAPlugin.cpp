#include <sndfile.h>

#include "../ARA/AraDebug.h"
#include "../ARA/ARAVST3.h"

#include "ARAPlugin.h"
#include "ARAUtil.h"

#include "../UniSampler/Source/Plugin/UniSampler.h"

using namespace ARA;

ARAAssertFunction assertFunction = &AraInterfaceAssert;
ARAAssertFunction * assertFunctionReference = &assertFunction;

ARAPlugin::ARAPlugin( UniSampler * pSampler ) :
	m_pSampler( pSampler )
	, m_pDocControllerInterface( nullptr )
	, m_pDocControllerRef( nullptr )
	, m_pPluginRef( nullptr )
	, m_pPluginExtensionInterface( nullptr )
	, m_pARAFactory( nullptr )
{
	AraSetExternalAssertReference( assertFunctionReference );
}

ARAPlugin::~ARAPlugin()
{
	clear();
}

// Try to load the plugin
bool ARAPlugin::LoadPlugin( std::string strPluginName )
{
	VST3Effect vst3Effect;
	if ( !dbgASSERT( vst3Effect.LoadBinary( strPluginName ), "Unable to load VST3 Effect", strPluginName ) )
		return false;

	// Create and verify factory
	const ARAFactory * factory = vst3Effect.GetARAFactory();
	if ( !dbgASSERT( factory && 
					 factory->structSize >= kARAFactoryMinSize &&
					 factory->lowestSupportedApiGeneration <= kARACurrentAPIGeneration &&
					 factory->highestSupportedApiGeneration >= kARACurrentAPIGeneration &&
					 factory->factoryID &&
					 strlen( factory->factoryID ) > 5 &&
					 factory->initializeARAWithConfiguration &&
					 factory->uninitializeARA &&
					 factory->createDocumentControllerWithDocument
					 , "Invalid or missing ARA factory" ) )
		return false;

	// Initialize ARA factory
	m_InterfaceConfig = { sizeof( m_InterfaceConfig ), kARACurrentAPIGeneration, assertFunctionReference };
	factory->initializeARAWithConfiguration( &m_InterfaceConfig );

	// Create ARA document
	ARA_LOG( "TestHost: creating a document and hooking up plug-in instance" );

	// Set this up such that we have access to the root sample as our ARAAudioAccessControllerHostRef
	std::unique_ptr<ARADocumentControllerHostInstance> upDocumentController( new ARADocumentControllerHostInstance ( { sizeof( ARADocumentControllerHostInstance), this, &hostAudioAccessControllerInterface,
																											  kArchivingControllerHostRef, &hostArchivingInterface,
																											  kContentAccessControllerHostRef, &hostContentAccessControllerInterface,
																											  kModelUpdateControllerHostRef, &hostModelUpdateControllerInterface,
																											  nullptr, nullptr } ) );

	std::unique_ptr<ARADocumentProperties> upDocumentProperties;
	upDocumentProperties.reset( new ARADocumentProperties( { sizeof( ARADocumentProperties ), "JUCE ARA Sampler" } ) );
	const ARADocumentControllerInstance * documentControllerInstance = factory->createDocumentControllerWithDocument( upDocumentController.get(), upDocumentProperties.get() );
	if ( !dbgASSERT( documentControllerInstance, "Unable to create document controller" ) )
		return false;

	const ARADocumentControllerRef documentControllerRef = documentControllerInstance->documentControllerRef;
	const ARADocumentControllerInterface * documentControllerInterface = documentControllerInstance->documentControllerInterface;
	if ( !dbgASSERT( documentControllerInterface &&
					 documentControllerInterface->structSize >= kARADocumentControllerInterfaceMinSize &&
					 ARA::AraCheckFunctionPointersAreValid( documentControllerInterface, documentControllerInstance->structSize )
					 , "Invalid document controller interface" ) )
		return false;

	// bind companion plug-in to document
	const ARAPlugInExtensionInstance *plugInInstance = vst3Effect.GetARAPlugInExtension( documentControllerRef );
	if ( !dbgASSERT( plugInInstance && plugInInstance->plugInExtensionRef, "Unable to bind plugin instance to document" ) )
		return false;

	// Make sure have the corrent plugin interface
	ARAPlugInExtensionRef plugInExtensionRef = plugInInstance->plugInExtensionRef;
	const ARAPlugInExtensionInterface * plugInExtensionInterface = plugInInstance->plugInExtensionInterface;
	if ( !dbgASSERT( plugInExtensionInterface &&
					 plugInExtensionInterface->structSize >= kARAPlugInExtensionInterfaceMinSize &&
					 ARA::AraCheckFunctionPointersAreValid( plugInExtensionInterface, plugInExtensionInterface->structSize )
					 , "Invalid plugin interface" ) )
		return false;

	// Document is OK, clear previous and assign members
	clear();
	m_upDocController = std::move( upDocumentController );
	m_upDocProperties = std::move( upDocumentProperties );
	m_vst3Effect = std::move( vst3Effect );
	m_pDocControllerInterface = documentControllerInterface;
	m_pDocControllerRef = documentControllerRef;
	m_pPluginRef = plugInExtensionRef;
	m_pPluginExtensionInterface = plugInExtensionInterface;
	m_pARAFactory = factory;
	m_strPluginName = strPluginName;
}

void ARAPlugin::clear()
{
	if ( m_pDocControllerInterface && m_pDocControllerRef )
		m_pDocControllerInterface->destroyDocumentController( m_pDocControllerRef );
	m_pDocControllerInterface = nullptr;
	m_pDocControllerRef = nullptr;

	if ( m_pARAFactory )
	{
		m_pARAFactory->uninitializeARA();
		m_pARAFactory = nullptr;
	}

	m_vst3Effect.Clear();
	m_upDocController.reset();
	m_upDocProperties.reset();
	m_pCurrentRootSample = nullptr;
}

bool ARAPlugin::CreateSamples( std::string strRootSample, ICommandListener * pCMDListener /*= nullptr*/ )
{
	// Construct root sample - its validity is only as long as this function
	UniSampler::Sample rootSamp;

	// Open the file
	SF_INFO fileInfo;
	SNDFILE  * pSndFile = sf_open( strRootSample.c_str(), SFM_READ, &fileInfo );
	if ( !dbgASSERT( pSndFile, "Unable to open sample file", strRootSample ) )
		return false;

	// Configure sample
	rootSamp.bStereo = (fileInfo.channels == 2);
	rootSamp.iSampleRate = fileInfo.samplerate;
	rootSamp.bFloat = (SF_FORMAT_FLOAT == (fileInfo.format & SF_FORMAT_SUBMASK));
	int iSampleCount = fileInfo.channels * fileInfo.frames;
	if ( rootSamp.bFloat )
	{
		rootSamp.vfDataL.resize( iSampleCount );
		ARA_VALIDATE_CONDITION( fileInfo.frames == sf_readf_float( pSndFile, rootSamp.vfDataL.data(), iSampleCount ) );

		if ( rootSamp.bStereo )
		{
			rootSamp.vfDataR.resize( fileInfo.frames );
			for ( int i = 0; i < fileInfo.frames; i++ )
			{
				rootSamp.vfDataR[i] = rootSamp.vfDataL[2 * i + 1];
				rootSamp.vfDataL[i] = rootSamp.vfDataL[2 * i];
			}
			rootSamp.vfDataL.resize( fileInfo.frames );
		}
	}
	// If the sample isn't float then we convert it to float
	else
	{
		rootSamp.vsDataL.resize( iSampleCount );
		ARA_VALIDATE_CONDITION( fileInfo.frames == sf_readf_short( pSndFile, rootSamp.vsDataL.data(), iSampleCount ) );

		rootSamp.vfDataL.resize( fileInfo.frames );

		if ( rootSamp.bStereo )
		{
			rootSamp.vfDataR.resize( fileInfo.frames );
			for ( int i = 0; i < fileInfo.frames; i++ )
			{
				rootSamp.vfDataR[i] = SHRT2FLT * rootSamp.vsDataL[2 * i + 1];
				rootSamp.vfDataL[i] = SHRT2FLT * rootSamp.vsDataL[2 * i];
			}
		}
		else
		{
			for ( int i = 0; i < fileInfo.frames; i++ )
			{
				rootSamp.vfDataL[i] = SHRT2FLT * rootSamp.vsDataL[i];;
			}
		}
		rootSamp.vsDataL.clear();
		rootSamp.bFloat = true;
	}

	// Close input file
	sf_close( pSndFile );

	std::string strSampleKey = str::ResolvePath( str::FixBackSlash( strRootSample ) );
	m_pCurrentRootSample = UniSampler::InjectSample( strSampleKey, rootSamp );

	if ( pCMDListener )
	{
		cmdSampleLoaded cmd;
		cmd.pSample = m_pCurrentRootSample;
		cmd.strSampleName = strRootSample;
		pCMDListener->HandleCommand( CmdPtr( new cmdSampleLoaded( cmd ) ) );
	}

	// start editing the document
	m_pDocControllerInterface->beginEditing( m_pDocControllerRef );

	// add an audio source to it and an audio modification to contain the edits for this source
	ARAAudioSourceProperties audioSourceProperties = { sizeof( audioSourceProperties ), "Test audio source", "audioSourceTestPersistentID",
		m_pCurrentRootSample->size(), (double) m_pCurrentRootSample->iSampleRate, fileInfo.channels, kARAFalse };

	// Create audio source and provide the root sample pointer as the host reference (we'll use it when the plugin reads the audio data)
	ARAAudioSourceRef audioSourceRef = m_pDocControllerInterface->createAudioSource( m_pDocControllerRef, this, &audioSourceProperties );

	// Create the audio modification so we can do the time stretch
	ARAAudioModificationProperties audioModificationProperties = { sizeof( ARAAudioModificationProperties ),
		"Test audio modification", "audioModificationTestPersistentID" };
	ARAAudioModificationRef audioModificationRef = m_pDocControllerInterface->createAudioModification( m_pDocControllerRef, audioSourceRef, kHostAudioModificationHostRef, &audioModificationProperties );

	// add a musical context to describe our timeline
	ARAMusicalContextProperties musicalContextProperties = { sizeof( musicalContextProperties ) };
	ARAMusicalContextRef musicalContextRef = m_pDocControllerInterface->createMusicalContext( m_pDocControllerRef, kHostMusicalContextHostRef, &musicalContextProperties );

	// Set up playback region for our sample
	double dRootDurInSeconds = (double) m_pCurrentRootSample->size() / (double) m_pCurrentRootSample->iSampleRate;
	ARAPlaybackRegionProperties playbackRegionProperties = { sizeof( playbackRegionProperties ), kARAPlaybackTransformationNoChanges,
		0, dRootDurInSeconds, 0, dRootDurInSeconds, musicalContextRef };

	playbackRegionProperties.transformationFlags |= kARAPlaybackTransformationTimestretch;

	// add a playback region to render modification in our musical context
	ARAPlaybackRegionRef playbackRegionRef = m_pDocControllerInterface->createPlaybackRegion( m_pDocControllerRef, audioModificationRef, kHostAudioModificationHostRef, &playbackRegionProperties );
	m_pPluginExtensionInterface->setPlaybackRegion( m_pPluginRef, playbackRegionRef );

	// done with editing the document, allow plug-in to access the audio
	m_pDocControllerInterface->endEditing( m_pDocControllerRef );
	m_pDocControllerInterface->enableAudioSourceSamplesAccess( m_pDocControllerRef, audioSourceRef, kARATrue );

	// Perform analysis - it's fine to do this inline because this whole process should be done on its own thread
	while ( 1 )
	{
		ARABool allDone = kARATrue;
		ARASize i;

		// this is a crude test implementation - real code wouldn't implement such a simple infinite loop.
		// instead, it would periodically request notifications and evaluate incoming calls to notifyAudioSourceContentChanged().
		// further, it would evaluate notifyAudioSourceAnalysisProgress() to provide proper progress indication,
		// and offer the user a way to cancel the operation if desired.
		m_pDocControllerInterface->notifyModelUpdates( m_pDocControllerRef );

		for ( i = 0; i < m_pARAFactory->analyzeableContentTypesCount; ++i )
		{
			if ( m_pDocControllerInterface->isAudioSourceContentAnalysisIncomplete( m_pDocControllerRef, audioSourceRef, m_pARAFactory->analyzeableContentTypes[i] ) )
			{
				allDone = kARAFalse;
				break;
			}
		}
		if ( allDone )
			break;

#if defined(_MSC_VER)
		Sleep( 100 );
#elif defined(__APPLE__)
		usleep( 100000 );
#endif
	}

	// Get note data - the sample should be a single note
	if ( dbgASSERT( m_pDocControllerInterface->isAudioSourceContentAvailable( m_pDocControllerRef, audioSourceRef, kARAContentTypeNotes ), "Unable to extract note data from sample", strRootSample ) )
	{
		// Create teh content reader so that we can extract note data from the source audio
		ARAContentReaderRef contentReaderRef = m_pDocControllerInterface->createAudioSourceContentReader( m_pDocControllerRef, audioSourceRef, kARAContentTypeNotes, NULL );
		ARAInt32 eventCount = m_pDocControllerInterface->getContentReaderEventCount( m_pDocControllerRef, contentReaderRef );

		// We need at least one note
		if ( dbgASSERT ( eventCount > 0, "No note data in sample", strRootSample ) )
		{
			// Get the note data
			const ARAContentNote * pNoteData = (const ARAContentNote *) m_pDocControllerInterface->getContentReaderDataForEvent( m_pDocControllerRef, contentReaderRef, 0 );
			if ( dbgASSERT( pNoteData, "Invalid note data in sample", strRootSample ) )
			{
				// Get source pitch and MIDI note, and create all other notes
				int iRootNote = pNoteData->pitchNumber;
				float fRootFreq = pNoteData->frequency;

				// We're going to be constructing an XML document in memory
				// This will be the synth patch that we create with our stretched samples
				using namespace tinyxml2;
				XMLElement * pGroupEl = nullptr;
				tinyxml2::XMLDocument doc;
				XMLElement * pGlobal = doc.NewElement( "global" );
				doc.InsertEndChild( pGlobal );
				pGroupEl = doc.NewElement( "group" );
				pGlobal->InsertEndChild( pGroupEl );
				pGroupEl->SetAttribute( "attack", 0.05f );
				pGroupEl->SetAttribute( "release", 0.5f );

				VST3Effect::RenderContext rc( &m_vst3Effect, m_pCurrentRootSample->bStereo );

				// For every MIDI note
// #pragma omp parallel for
				for ( int n = 0; n < 128; n++ )
				{
					// Skip the root note, add it to the pool as is
					std::string strStretchedKey = strSampleKey;
					if ( n != iRootNote )
					{
						strStretchedKey = str::StripExtension( strSampleKey ) + to_str( n ) + str::GetExtension( strRootSample );

						// Create and configure new sample
						double dRatio = pow( 2, (n - (double) iRootNote) / 12. );
						int iNewSampleCount = dRatio * m_pCurrentRootSample->size() + 0.5;
						float * apStretched[2]{ nullptr };
						std::vector<float> vStretched[2];
						for ( int c = 0; c < fileInfo.channels; c++ )
						{
							vStretched[c].resize( iNewSampleCount );
							apStretched[c] = vStretched[c].data();
						}

// #pragma omp critical
						{
							// Stretch the sample by its pitch ratio relative to the root
							m_pDocControllerInterface->beginEditing( m_pDocControllerRef );;
							playbackRegionProperties.durationInPlaybackTime = dRatio * dRootDurInSeconds;
							m_pDocControllerInterface->updatePlaybackRegionProperties( m_pDocControllerRef, playbackRegionRef, &playbackRegionProperties );
							m_pDocControllerInterface->endEditing( m_pDocControllerRef );

							int nBlockSize = 1024;
							int nBlocks = iNewSampleCount / nBlockSize + (int) (iNewSampleCount % nBlockSize != 0);
							int nSamplesLeft = iNewSampleCount;
							int iHostPos = 0;
							for ( ; nSamplesLeft > 0; nSamplesLeft -= nBlockSize )
							{
								float * apProcess[2] = { apStretched[0] + iHostPos, (apStretched[1] ? apStretched[1] + iHostPos : nullptr) };
								int nSamplesToProcess = dsp::Clamp( nSamplesLeft, 0, nBlockSize );
								m_vst3Effect.Process( dsp::Clamp( nSamplesLeft, 0, nBlockSize ), fileInfo.samplerate, iHostPos, apProcess, fileInfo.channels );
								iHostPos += nSamplesToProcess;
							}
						}

						UniSampler::Sample sampleStretched;
						sampleStretched.iSampleRate = m_pCurrentRootSample->iSampleRate;
						sampleStretched.bFloat = m_pCurrentRootSample->bFloat;
						sampleStretched.bStereo = m_pCurrentRootSample->bStereo;
						sampleStretched.vfDataL.resize( m_pCurrentRootSample->vfDataL.size() );
						sampleStretched.vfDataR.resize( m_pCurrentRootSample->vfDataR.size() );

						float * apOutput[2] = { sampleStretched.vfDataL.data(), sampleStretched.vfDataR.data() };
						for ( int c = 0; c < fileInfo.channels; c++ )
							dsp::Resample_Cubic( apOutput[c], apStretched[c], 0, dRatio, m_pCurrentRootSample->size(), iNewSampleCount );

						// Now add the stretched and then repitched sample to the pool
						UniSampler::InjectSample( strStretchedKey, sampleStretched );

						if ( pCMDListener )
						{
							cmdRegionCreated cmd;
							cmd.strSampleName = strStretchedKey;
							cmd.iMIDINote = n;
							cmd.strSampleName = strRootSample;
							pCMDListener->HandleCommand( CmdPtr( new cmdRegionCreated( cmd ) ) );
						}
					}

					// Add the sample to the XML doc as a region
// #pragma omp critical
					{
						XMLElement *pRegionEl = doc.NewElement( "region" );
						pRegionEl->SetAttribute( "sample", strStretchedKey.c_str() );
						pRegionEl->SetAttribute( "key_d", n );
						pGroupEl->InsertEndChild( pRegionEl );
					}
				}

				// Now the document should be complete and all samples stretched / stored
				// Set the XML program
				m_pSampler->LoadXML( "ARAStretchedSamples.xml", &doc );
				if ( pCMDListener )
				{
					pCMDListener->HandleCommand( CmdPtr( new cmdProgramSet() ) );
				}
			}
		}
	}

	// We're done, remove the playback region
	m_pDocControllerInterface->enableAudioSourceSamplesAccess( m_pDocControllerRef, audioSourceRef, kARAFalse );
	m_pPluginExtensionInterface->removePlaybackRegion( m_pPluginRef, playbackRegionRef );
	m_pDocControllerInterface->beginEditing( m_pDocControllerRef );
	m_pDocControllerInterface->destroyPlaybackRegion( m_pDocControllerRef, playbackRegionRef );
	m_pDocControllerInterface->destroyAudioModification( m_pDocControllerRef, audioModificationRef );
	m_pDocControllerInterface->destroyAudioSource( m_pDocControllerRef, audioSourceRef );
	m_pDocControllerInterface->destroyMusicalContext( m_pDocControllerRef, musicalContextRef );
	m_pDocControllerInterface->endEditing( m_pDocControllerRef );
}

UniSampler::Sample * ARAPlugin::GetCurrentRootSample() const
{
	return m_pCurrentRootSample;
}