#pragma once

/***************************************************************
ARAUtil.h

This is essentially a copy of the ARAExampleUtil.h file from
the ARA SDK modified to work with my code. Specifically we need
valid pointers to the ARAAudioSourceHostRef and ARAContentAccessControllerHostRef
objects so that we can access our audio sample data

***************************************************************/

#include "../ARA/ARAInterface.h"
#include "../ARA/AraDebug.h"

// Leave these as is for validation purposes save for the 
// ARAAudioSourceHostRef and ARAContentAccessControllerHostRef
//======================================================================================
// #define kHostAudioSourceHostRef ((ARA::ARAAudioSourceHostRef) 1)
#define kHostAudioModificationHostRef ((ARA::ARAAudioModificationHostRef) 2)
#define kHostMusicalContextHostRef ((ARA::ARAMusicalContextHostRef) 3)
// #define kAudioAccessControllerHostRef ((ARA::ARAContentAccessControllerHostRef) 10)
#define kContentAccessControllerHostRef ((ARA::ARAContentAccessControllerHostRef) 11)
#define kModelUpdateControllerHostRef ((ARA::ARAModelUpdateControllerHostRef) 12)
#define kPlaybackControllerHostRef ((ARA::ARAPlaybackControllerHostRef) 13)
#define kArchivingControllerHostRef ((ARA::ARAArchivingControllerHostRef) 14)
#define kAudioAccessor32BitHostRef ((ARA::ARAAudioReaderHostRef) 20)
#define kAudioAccessor64BitHostRef ((ARA::ARAAudioReaderHostRef) 21)
#define kHostTempoContentReaderHostRef ((ARA::ARAContentReaderHostRef) 30)
#define kHostSignaturesContentReaderHostRef ((ARA::ARAContentReaderHostRef) 31)
#define kARAInvalidHostRef ((ARA::ARAHostRef) 0)

// This is needed in C++
using namespace ARA;

// ARAAudioAccessControllerInterface (required)
ARAAudioReaderHostRef ARA_CALL ARACreateAudioReaderForSource( ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARABool use64BitSamples )
{
	ARAAudioReaderHostRef accessorRef = (use64BitSamples) ? kAudioAccessor64BitHostRef : kAudioAccessor32BitHostRef;
	// ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef );
	dbgASSERT( controllerHostRef, "Missing Controller Host Ref to our sample" );
	//ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );
	ARA_LOG( "TestHost: createAudioReaderForSource() returns fake ref %p", accessorRef );
	return accessorRef;
}

// Similar to the SDK example, but the input data from the host is our ARA Plugin instance
ARABool ARA_CALL ARAReadAudioSamples( ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioReaderHostRef readerHostRef,
									  ARASamplePosition samplePosition, ARASampleCount samplesPerChannel, void * const * buffers )
{
	void * bufferPtr;

	// ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef );

	// The controllerHostRef is a pointer to our ARAPlugin instance, which has a sample member containing our audio
	if ( dbgASSERT( controllerHostRef, "Missing Controller Host Ref to our sample" ) )
	{
		// Get sample
		ARAResampler * pPlugin = (ARAResampler *) controllerHostRef;
		UniSampler::Sample * pSample = pPlugin->GetCurrentRootSample();

		// Validate arguments
		ARA_VALIDATE_ARGUMENT( readerHostRef, (readerHostRef == kAudioAccessor32BitHostRef) || (readerHostRef == kAudioAccessor64BitHostRef) );
		ARA_VALIDATE_ARGUMENT( NULL, 0 <= samplePosition );
		ARA_VALIDATE_ARGUMENT( NULL, samplePosition + samplesPerChannel <= pSample->size() );
		ARA_VALIDATE_ARGUMENT( buffers, buffers != NULL );

		// Get per channel audio
		int iChannelCount = pSample->bStereo ? 2 : 1;
		for ( int c = 0; c < iChannelCount; c++ )
		{
			ARA_VALIDATE_ARGUMENT( buffers, buffers[c] != NULL );

			bufferPtr = buffers[c];
			for ( int i = 0; i < samplesPerChannel; i++ )
			{
				// Read the value from our sample relative to host sample pos, convert from short if necessary
				double value = pSample->at( c, samplePosition + i );
				if ( pSample->bFloat == false )
					value /= SHRT_MAX;

				if ( readerHostRef == kAudioAccessor64BitHostRef )
				{
					double * doublePtr = (double *) bufferPtr;
					*doublePtr++ = value;
					bufferPtr = doublePtr;
				}
				else
				{
					float * floatPtr = (float *) bufferPtr;
					*floatPtr++ = (float) value;
					bufferPtr = floatPtr;
				}
			}
		}

		return kARATrue;
	}

	return kARAFalse;
}
void ARA_CALL ARADestroyAudioReader( ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioReaderHostRef readerHostRef )
{
	// ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef );
	dbgASSERT( controllerHostRef, "Missing Controller Host Ref to our sample" );
	ARA_VALIDATE_ARGUMENT( readerHostRef, (readerHostRef == kAudioAccessor32BitHostRef) || (readerHostRef == kAudioAccessor64BitHostRef) );
	ARA_LOG( "TestHost: destroyAudioReader() called for fake ref %p", readerHostRef );
}
static const ARAAudioAccessControllerInterface hostAudioAccessControllerInterface = { sizeof( hostAudioAccessControllerInterface ),
&ARACreateAudioReaderForSource, &ARAReadAudioSamples, &ARADestroyAudioReader };


// ARAContentAccessControllerInterface
ARABool ARA_CALL ARAIsMusicalContextContentAvailable( ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef );

	if ( type == kARAContentTypeTempoEntries )
		return kARATrue;

	if ( type == kARAContentTypeSignatures )
		return kARATrue;

	return kARAFalse;
}
ARAContentGrade ARA_CALL ARAGetMusicalContextContentGrade( ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef );

	if ( type == kARAContentTypeTempoEntries )
		return kARAContentGradeAdjusted;

	if ( type == kARAContentTypeSignatures )
		return kARAContentGradeAdjusted;

	return kARAContentGradeInitial;
}
ARAContentReaderHostRef ARA_CALL ARACreateMusicalContextContentReader( ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type, const ARAContentTimeRange * range )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef );
	ARA_VALIDATE_ARGUMENT( NULL, (type == kARAContentTypeTempoEntries) || (type == kARAContentTypeSignatures) );	// only supported types may be requested

	if ( type == kARAContentTypeTempoEntries )
	{
		ARA_LOG( "TestHost: createMusicalContextContentReader() called for fake tempo reader ref %p", kHostTempoContentReaderHostRef );
		return kHostTempoContentReaderHostRef;
	}

	if ( type == kARAContentTypeSignatures )
	{
		ARA_LOG( "TestHost: createMusicalContextContentReader() called for fake signatures reader ref %p", kHostSignaturesContentReaderHostRef );
		return kHostSignaturesContentReaderHostRef;
	}

	return kARAInvalidHostRef;
}
ARABool ARA_CALL ARAIsAudioSourceContentAvailable( ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	// ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );

	return kARAFalse;
}
ARAContentGrade ARA_CALL ARAGetAudioSourceContentGrade( ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	// ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );

	return kARAContentGradeInitial;
}
ARAContentReaderHostRef ARA_CALL ARACreateAudioSourceContentReader( ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type, const ARAContentTimeRange * range )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	// ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );

	ARA_VALIDATE_CONDITION( 0 && "should never be called since we do not provide content information at audio source level" );

	return kARAInvalidHostRef;
}


static const ARAContentSignature signatureDefinition = { 4, 4, 0.0 };
static const ARAContentTempoEntry tempoSyncPoints[2] = { { 0.0, 0.0 },{ 0.5, 1.0 } };
// these are some more valid timelines you can use for testing your implementation:
// static const ARAContentTempoEntry tempoSyncPoints[2] = { { -0.5, -1.0 }, { 0.0, 0.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[4] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[4] = { { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[5] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };

ARAInt32 ARA_CALL ARAGetContentReaderEventCount( ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef) );

	if ( readerHostRef == kHostTempoContentReaderHostRef )
		return sizeof( tempoSyncPoints ) / sizeof( tempoSyncPoints[0] );

	if ( readerHostRef == kHostSignaturesContentReaderHostRef )
		return 1;

	return 0;
}
const void * ARA_CALL ARAGetContentReaderDataForEvent( ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef, ARAInt32 eventIndex )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef) );
	ARA_VALIDATE_ARGUMENT( NULL, 0 <= eventIndex );
	ARA_VALIDATE_ARGUMENT( NULL, eventIndex < ARAGetContentReaderEventCount( controllerHostRef, readerHostRef ) );

	if ( readerHostRef == kHostTempoContentReaderHostRef )
	{
		return &tempoSyncPoints[eventIndex];
	}

	if ( readerHostRef == kHostSignaturesContentReaderHostRef )
	{
		return &signatureDefinition;
	}

	return NULL;
}
void ARA_CALL ARADestroyContentReader( ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kContentAccessControllerHostRef );
	ARA_VALIDATE_ARGUMENT( readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef) );
	ARA_LOG( "TestHost: plug-in destroyed content reader ref %p", readerHostRef );
}
static const ARAContentAccessControllerInterface hostContentAccessControllerInterface = { sizeof( hostContentAccessControllerInterface ),
&ARAIsMusicalContextContentAvailable, &ARAGetMusicalContextContentGrade, &ARACreateMusicalContextContentReader,
&ARAIsAudioSourceContentAvailable, &ARAGetAudioSourceContentGrade, &ARACreateAudioSourceContentReader,
&ARAGetContentReaderEventCount, &ARAGetContentReaderDataForEvent, &ARADestroyContentReader };


// ARAModelUpdateControllerInterface
#define kInvalidProgressValue -1.0f
static float lastProgressValue = kInvalidProgressValue;
void ARA_CALL ARANotifyAudioSourceAnalysisProgress( ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAAnalysisProgressState state, float value )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef );
	// ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );
	ARA_VALIDATE_ARGUMENT( NULL, 0.0f <= value );
	ARA_VALIDATE_ARGUMENT( NULL, value <= 1.0f );
	ARA_VALIDATE_ARGUMENT( NULL, lastProgressValue <= value );

	switch ( state )
	{
		case kARAAnalysisProgressStarted:
		{
			ARA_VALIDATE_STATE( lastProgressValue == kInvalidProgressValue );
			ARA_LOG( "TestHost: audio source analysis started with progress %.f%%.", 100.0f * value );
			lastProgressValue = value;
			break;
		}
		case kARAAnalysisProgressUpdated:
		{
			ARA_VALIDATE_STATE( lastProgressValue != kInvalidProgressValue );
			ARA_LOG( "TestHost: audio source analysis progresses %.f%%.", 100.0f * value );
			lastProgressValue = value;
			break;
		}
		case kARAAnalysisProgressCompleted:
		{
			ARA_VALIDATE_STATE( lastProgressValue != kInvalidProgressValue );
			ARA_LOG( "TestHost: audio source analysis finished with progress %.f%%.", 100.0f * value );
			lastProgressValue = kInvalidProgressValue;
			break;
		}
		default:
		{
			ARA_VALIDATE_ARGUMENT( NULL, 0 && "invalid progress state" );
		}
	}
}
void ARA_CALL ARANotifyAudioSourceContentChanged( ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, const ARAContentTimeRange * range, ARAContentUpdateFlags contentFlags )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef );
	// ARA_VALIDATE_ARGUMENT( audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef );
	ARAResampler * pPlugin = (ARAResampler *) audioSourceHostRef;
	UniSampler::Sample * pSample = pPlugin->GetCurrentRootSample();
	dbgASSERT( audioSourceHostRef, "Missing Audio Source Host Ref to our sample" );
	if ( range )
	{
		ARA_VALIDATE_ARGUMENT( range, 0.0 <= range->start );
		ARA_VALIDATE_ARGUMENT( range, 0.0 <= range->duration );
		ARA_VALIDATE_ARGUMENT( range, range->start + range->duration < pSample->durInSeconds() );
	}
	ARA_LOG( "TestHost: audio source content was updated in range %.3f-%.3f, flags %X", (range) ? range->start : 0.0, (range) ? range->start + range->duration : pSample->durInSeconds(), contentFlags );
}
void ARA_CALL ARANotifyAudioModificationContentChanged( ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioModificationHostRef audioModificationHostRef, const ARAContentTimeRange * range, ARAContentUpdateFlags contentFlags )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef );
	ARA_VALIDATE_ARGUMENT( audioModificationHostRef, audioModificationHostRef == kHostAudioModificationHostRef );
	if ( range )
	{
		ARA_VALIDATE_ARGUMENT( range, 0.0 <= range->duration );

		ARA_LOG( "TestHost: audio modification content was updated in range %.3f-%.3f, flags %X", (range) ? range->start : 0.0, (range) ? range->start + range->duration : -1, contentFlags );
	}
}
static const ARAModelUpdateControllerInterface hostModelUpdateControllerInterface = { sizeof( hostModelUpdateControllerInterface ),
&ARANotifyAudioSourceAnalysisProgress, &ARANotifyAudioSourceContentChanged, &ARANotifyAudioModificationContentChanged };


// ARAPlaybackControllerInterface
void ARA_CALL ARARequestStartPlayback( ARAPlaybackControllerHostRef controllerHostRef )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kPlaybackControllerHostRef );
	ARA_WARN( "TestHost: requestStartPlayback not implemented." );
}
void ARA_CALL ARARequestStopPlayback( ARAPlaybackControllerHostRef controllerHostRef )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kPlaybackControllerHostRef );
	ARA_WARN( "TestHost: requestStopPlayback not implemented" );
}
void ARA_CALL ARARequestSetPlaybackPosition( ARAPlaybackControllerHostRef controllerHostRef, ARATimePosition timePosition )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kPlaybackControllerHostRef );
	ARA_WARN( "TestHost: requestSetPlaybackPosition() not implemented, requested time is %.2f", timePosition );
}
void ARA_CALL ARARequestSetCycleRange( ARAPlaybackControllerHostRef controllerHostRef, ARATimePosition startTime, ARATimeDuration duration )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kPlaybackControllerHostRef );
	ARA_WARN( "TestHost: requestSetCycleRange() not implemented, requested range is %.2f to %.2f", startTime, startTime + duration );
}
void ARA_CALL ARARequestEnableCycle( ARAPlaybackControllerHostRef controllerHostRef, ARABool enable )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kPlaybackControllerHostRef );
	ARA_WARN( "TestHost: requestEnableCycle() not implemented, requested to turn %s.", (enable != kARAFalse) ? "on" : "off" );
}
static const ARAPlaybackControllerInterface hostPlaybackControllerInterface = { kARAPlaybackControllerInterfaceMinSize /* we're not supporting everything here: sizeof(hostPlaybackControllerInterface) */,
&ARARequestStartPlayback, &ARARequestStopPlayback, &ARARequestSetPlaybackPosition, &ARARequestSetCycleRange, &ARARequestEnableCycle };


// ARAArchivingControllerInterface
typedef struct TestArchive
{
	char * path;
	FILE * file;
} TestArchive;

ARASize ARA_CALL ARAGetArchiveSize( ARAArchivingControllerHostRef controllerHostRef, ARAArchiveReaderHostRef archiveReaderHostRef )
{
	TestArchive * archive = (TestArchive *) archiveReaderHostRef;
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kArchivingControllerHostRef );
	ARA_VALIDATE_ARGUMENT( archiveReaderHostRef, archive != NULL );

	fseek( archive->file, 0L, SEEK_END );
	return (ARASize) ftell( archive->file );
}
ARABool ARA_CALL ARAReadBytesFromArchive( ARAArchivingControllerHostRef controllerHostRef, ARAArchiveReaderHostRef archiveReaderHostRef,
										  ARASize position, ARASize length, void * const buffer )
{
	size_t value;
	TestArchive * archive = (TestArchive *) archiveReaderHostRef;
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kArchivingControllerHostRef );
	ARA_VALIDATE_ARGUMENT( archiveReaderHostRef, archive != NULL );
	ARA_VALIDATE_ARGUMENT( NULL, 0 < length );
	ARA_VALIDATE_ARGUMENT( NULL, position + length <= ARAGetArchiveSize( controllerHostRef, archiveReaderHostRef ) );

	value = fseek( archive->file, (long) position, SEEK_SET );
	ARA_INTERNAL_ASSERT( value == position );

	value = fread( buffer, 1, length, archive->file );
	ARA_INTERNAL_ASSERT( value == length );

	return kARATrue;
}
ARABool ARA_CALL ARAWriteBytesToArchive( ARAArchivingControllerHostRef controllerHostRef, ARAArchiveWriterHostRef archiveWriterHostRef,
										 ARASize position, ARASize length, const void * const buffer )
{
	size_t value;
	TestArchive * archive = (TestArchive *) archiveWriterHostRef;
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kArchivingControllerHostRef );
	ARA_VALIDATE_ARGUMENT( archiveWriterHostRef, archive != NULL );
	ARA_VALIDATE_ARGUMENT( NULL, 0 < length );

	value = fseek( archive->file, (long) position, SEEK_SET );
	ARA_INTERNAL_ASSERT( value == position );

	value = fwrite( buffer, 1, length, archive->file );
	ARA_INTERNAL_ASSERT( value == length );

	return kARATrue;
}
void ARA_CALL ARANotifyDocumentArchivingProgress( ARAArchivingControllerHostRef controllerHostRef, float value )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kArchivingControllerHostRef );
	ARA_LOG( "TestHost: document archiving progresses %.f%%.", 100.0f * value );
}
void ARA_CALL ARANotifyDocumentUnarchivingProgress( ARAArchivingControllerHostRef controllerHostRef, float value )
{
	ARA_VALIDATE_ARGUMENT( controllerHostRef, controllerHostRef == kArchivingControllerHostRef );
	ARA_LOG( "TestHost: document unarchiving progresses %.f%%.", 100.0f * value );
}
static const ARAArchivingControllerInterface hostArchivingInterface = { kARAArchivingControllerInterfaceMinSize,
&ARAGetArchiveSize, &ARAReadBytesFromArchive, &ARAWriteBytesToArchive,
&ARANotifyDocumentArchivingProgress, &ARANotifyDocumentUnarchivingProgress };

