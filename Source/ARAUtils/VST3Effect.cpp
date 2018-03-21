//------------------------------------------------------------------------------
//! \file        vst3loader.cpp
//! \description VST3 specific ARA implementation for the SDK minihost example
//! \project     ARA SDK, examples
//------------------------------------------------------------------------------
// Copyright (c) 2012-2013, Celemony Software GmbH, All Rights Reserved.
// 
// License & Disclaimer:
// 
// This Software Development Kit (SDK) may not be distributed in parts or 
// its entirety without prior written agreement of Celemony Software GmbH. 
//
// This SDK must not be used to modify, adapt, reproduce or transfer any 
// software and/or technology used in any Celemony and/or Third-party 
// application and/or software module (hereinafter referred as "the Software"); 
// to modify, translate, reverse engineer, decompile, disassemble or create 
// derivative works based on the Software (except to the extent applicable laws 
// specifically prohibit such restriction) or to lease, assign, distribute or 
// otherwise transfer rights to the Software.
// Neither the name of the Celemony Software GmbH nor the names of its 
// contributors may be used to endorse or promote products derived from this 
// SDK without specific prior written permission.
//
// This SDK is provided by Celemony Software GmbH "as is" and any express or 
// implied warranties, including, but not limited to, the implied warranties of 
// non-infringement, merchantability and fitness for a particular purpose are
// disclaimed.
// In no event shall Celemony Software GmbH be liable for any direct, indirect, 
// incidental, special, exemplary, or consequential damages (including, but not 
// limited to, procurement of substitute goods or services; loss of use, data, 
// or profits; or business interruption) however caused and on any theory of 
// liability, whether in contract, strict liability, or tort (including 
// negligence or otherwise) arising in any way out of the use of this software, 
// even if advised of the possibility of such damage.
// Where the liability of Celemony Software GmbH is ruled out or restricted, 
// this will also apply for the personal liability of employees, representatives 
// and vicarious agents.
// The above restriction of liability will not apply if the damages suffered
// are attributable to willful intent or gross negligence or in the case of 
// physical injury.
//------------------------------------------------------------------------------

#include "VST3Effect.h"

#define INIT_CLASS_IID

#include "../ARA/AraDebug.h"
#include "../ARA/ARAVST3.h"
#include "../ARA/ARAInterface.h"
using namespace ARA;

#include "base/source/fstring.h"

#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#undef INIT_CLASS_IID

namespace Steinberg
{
	FUnknown* gStandardPluginContext = 0;
}

using namespace Steinberg;
using namespace Vst;

#if defined(_MSC_VER)
#include <windows.h>
#include <conio.h>
extern "C"
{
	typedef bool (PLUGIN_API *InitModuleProc) ();
	typedef bool (PLUGIN_API *ExitModuleProc) ();
}
static const Steinberg::FIDString kInitModuleProcName = "InitDll";
static const Steinberg::FIDString kExitModuleProcName = "ExitDll";
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <curses.h>
typedef bool ( *bundleEntryPtr )(CFBundleRef);
typedef bool ( *bundleExitPtr )(void);
#endif

VST3Effect::VST3Effect() :
	m_libHandle( nullptr ),
	m_pComponent( nullptr )
{}

VST3Effect::VST3Effect( VST3Effect&& other )
{
	m_libHandle = other.m_libHandle;
	other.m_libHandle = nullptr;
	m_pComponent = other.m_pComponent;
	other.m_pComponent = nullptr;
	m_strPluginName = other.m_strPluginName;
	other.m_strPluginName.clear();
}

VST3Effect& VST3Effect::operator=( VST3Effect&& other )
{
	m_libHandle = other.m_libHandle;
	other.m_libHandle = nullptr;
	m_pComponent = other.m_pComponent;
	other.m_pComponent = nullptr;
	m_strPluginName = other.m_strPluginName;
	other.m_strPluginName.clear();
	return *this;
}

VST3Effect::~VST3Effect() 
{
	Clear();
}

IComponent * VST3Effect::GetComponent() const
{
	return m_pComponent;
}

bool VST3Effect::isValid() const
{
	return m_libHandle && m_pComponent;
}

void VST3Effect::Clear()
{
	if ( m_pComponent )
	{
		tresult result = m_pComponent->terminate();
		m_pComponent->release();
		dbgASSERT( result == kResultOk, "Error terminating component for plugin", m_strPluginName );
		m_pComponent = nullptr;
	}

	if ( m_libHandle )
	{
#if defined(_MSC_VER)
		ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress( m_libHandle, kExitModuleProcName );
		if ( dbgASSERT( exitProc, "Missing exit proc in plugin", m_strPluginName ) )
			exitProc();
		FreeLibrary( m_libHandle );
#elif defined(__APPLE__)
		bundleExitPtr bundleExit = (bundleExitPtr) CFBundleGetFunctionPointerForName( m_libHandle, CFSTR( "bundleExit" ) );
		if ( dbgASSERT( bundleExit, "Missing exit proc in plugin", m_strPluginName ) )
			bundleExit();
		CFRelease( m_libHandle );
#endif
		m_libHandle = nullptr;
	}

	m_strPluginName.clear();
}

VST3Effect::RenderContext::RenderContext( VST3Effect * pVST3Effect, bool bStereo ) :
	m_pComponent( nullptr )
{
	if ( dbgASSERT( pVST3Effect, "Missing VST3 Plugin for render context" ) )
	{
		IComponent * pPlugComponent = pVST3Effect->GetComponent();
		if ( dbgASSERT( pPlugComponent, "Invalid plugin component for rendering context" ) )
		{
			IAudioProcessor * processor = NULL;
			tresult result = pPlugComponent->queryInterface( IAudioProcessor::iid, (void**) &processor );
			if ( dbgASSERT( result == kResultOk && processor, "Unable to get plugin processor" ) )
			{
				ProcessSetup setup = { kRealtime, kSample32, 1024, 44100.0 };
				result = processor->setupProcessing( setup );
				if ( dbgASSERT( result == kResultOk, "Unable to setup plugin processor" ) )
				{
					SpeakerArrangement inputs = bStereo ? SpeakerArr::kStereo : SpeakerArr::kMono;
					SpeakerArrangement outputs = bStereo ? SpeakerArr::kStereo : SpeakerArr::kMono;
					result = processor->setBusArrangements( &inputs, 1, &outputs, 1 );
					if ( dbgASSERT( result == kResultOk, "Unable to set bus arrangement" ) )
					{
						result = pPlugComponent->activateBus( kAudio, kInput, 0, true );
						if ( dbgASSERT( result == kResultOk, "Unable to activate input bus" ) )
						{
							result = pPlugComponent->activateBus( kAudio, kOutput, 0, true );
							if ( dbgASSERT( result == kResultOk, "Unable to activate output bus" ) )
							{
								result = pPlugComponent->setActive( true );
								if ( dbgASSERT( result == kResultOk, "Unable to activate plugin component" ) )
								{
									m_pComponent = pPlugComponent;
								}
								else
								{
									result = pPlugComponent->activateBus( kAudio, kInput, 0, false );
									result = pPlugComponent->activateBus( kAudio, kOutput, 0, false );
								}
							}
							else
							{
								result = pPlugComponent->activateBus( kAudio, kInput, 0, false );
							}
						}
					}
				}
				processor->release();
			}
		}
	}
}

VST3Effect::RenderContext::~RenderContext()
{
	if ( m_pComponent )
	{
		tresult result = m_pComponent->setActive( false );
		if ( dbgASSERT( result == kResultOk, "Unable to deactivate plugin component" ) )
		{
			result = m_pComponent->activateBus( kAudio, kInput, 0, false );
			if ( dbgASSERT( result == kResultOk, "Unable to deactivate input bus" ) )
			{
				result = m_pComponent->activateBus( kAudio, kOutput, 0, false );
				dbgASSERT( result == kResultOk, "Unable to deactivate output bus" );
			}
		}

	}
}

bool VST3Effect::LoadBinary( std::string strPluginName )
{
	bool bLoadSuccess = false;

	// load library
#if defined(_MSC_VER)
	HMODULE libHandle = ::LoadLibraryA( strPluginName.c_str() );
#elif defined(__APPLE__)
	CFURLRef url = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8*) strPluginName.c_str(), strPluginName.size(), true );
	if ( !dbgASSERT( url, "Unable to create file system URL for plugin", strPluginName ) )
	{
		CFBundleRef libHandle = CFBundleCreate( NULL, url );
		CFRelease( url );
	}
#endif
	GetFactoryProc entryProc = NULL;
	bool bPluginInit = false;
	if ( dbgASSERT( libHandle, "Unable to load plugin binary", strPluginName ) )
	{
#if defined(_MSC_VER)
		InitModuleProc initProc = (InitModuleProc)::GetProcAddress( libHandle, kInitModuleProcName );
		if ( dbgASSERT( initProc, "Unable to get init proc" ) )
		{
			bPluginInit = initProc();
			if ( dbgASSERT( bPluginInit, "Unable to initialize plugin binary" ) )
			{
				entryProc = (GetFactoryProc)::GetProcAddress( libHandle, "GetPluginFactory" );
			}
		}
#elif defined(__APPLE__)
		bundleEntryPtr bundleEntry = (bundleEntryPtr) CFBundleGetFunctionPointerForName( libHandle, CFSTR( "bundleEntry" ) );
		if ( dbgASSERT( bundleEntry, "Unable to get entry proc" ) )
		{
			bPluginInit = bundleEntry( libHandle )();
			if ( dbgASSERT( bPluginInit, "Unable to initialize plugin binary" ) )
			{
				entryProc = (GetFactoryProc) CFBundleGetFunctionPointerForName( libHandle, CFSTR( "GetPluginFactory" ) );
			}
		}
#endif
	}

	if ( dbgASSERT( entryProc, "Missing entryProc for plugin", strPluginName ) )
	{
		IPluginFactory * pFactory = entryProc();
		if ( dbgASSERT( pFactory, "Unable to create VST3 Plugin factory for plugin", strPluginName ) )
		{
			PFactoryInfo factoryInfo;
			tresult result = pFactory->getFactoryInfo( &factoryInfo );
			if ( dbgASSERT( result == Steinberg::kResultOk, "Error getting factory info for plugin", strPluginName ) )
			{
				// find component
				for ( int32 i = 0; i < pFactory->countClasses(); i++ )
				{
					PClassInfo classInfo;
					result = pFactory->getClassInfo( i, &classInfo );
					if ( dbgASSERT( result == Steinberg::kResultOk, "Error getting class info", i, "from plugin", strPluginName ) )
					{
						// Find the actual aduio effect class
						if ( strcmp( kVstAudioEffectClass, classInfo.category ) == 0 )
						{
							// create only the component part for the purpose of this test code
							IComponent * pComponent = nullptr;
							tresult result = pFactory->createInstance( classInfo.cid, IComponent::iid, (void**) &pComponent );
							if ( dbgASSERT( result == kResultOk && pComponent, "Unable to get plugin component from", strPluginName ) )
							{
								// initialize the component with our context
								result = pComponent->initialize( gStandardPluginContext );
								if ( dbgASSERT( result == Steinberg::kResultOk, "Unable to initialize plugin component for", strPluginName ) )
								{
									// We made it, clear and assign
									dbgTRACE( "Successfully loaded VST3 Plugin", strPluginName );
									Clear();
									m_libHandle = libHandle;
									m_pComponent = pComponent;
									m_strPluginName = strPluginName;
									bLoadSuccess = true;
									break;
								}
							}
							// Release component if we aren't using it
							else
							{
								if ( pComponent )
								{
									pComponent->terminate();
									pComponent->release();
								}
							}
						}
					}
				}
			}

			// Release factory
			pFactory->release();
		}
	}

	// Failure, uninit if necessary and release DLL
	if ( bLoadSuccess == false )
	{
		if ( libHandle )
		{
#if defined(_MSC_VER)
			if ( bPluginInit )
			{
				ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress( libHandle, kExitModuleProcName );
				if ( exitProc )
					exitProc();
			}
			FreeLibrary( libHandle );
#elif defined(__APPLE__)
			if ( bPluginInit )
			{
				bundleExitPtr bundleExit = (bundleExitPtr) CFBundleGetFunctionPointerForName( vst3Effect->libHandle, CFSTR( "bundleExit" ) );
				if ( bundleExit )
					bundleExit();
			}
			CFRelease( libHandle );
#endif
			libHandle = nullptr;
		}
	}

	return bLoadSuccess;
}


const ARA::ARAFactory * VST3Effect::GetARAFactory()
{
	const ARA::ARAFactory * result = NULL;

	ARA::IPlugInEntryPoint * entry = NULL;
	if ( (m_pComponent->queryInterface( ARA::IPlugInEntryPoint::iid, (void**) &entry ) == kResultTrue) && entry )
	{
		result = entry->getFactory();
		entry->release();
	}

	return result;
}

const ARA::ARAPlugInExtensionInstance * VST3Effect::GetARAPlugInExtension( ARA::ARADocumentControllerRef controllerRef )
{
	const ARA::ARAPlugInExtensionInstance * result = NULL;

	ARA::IPlugInEntryPoint * entry = NULL;
	if ( (m_pComponent->queryInterface( ARA::IPlugInEntryPoint::iid, (void**) &entry ) == kResultTrue) && entry )
	{
		result = entry->bindToDocumentController( controllerRef );
		entry->release();
	}

	return result;
}

void VST3Effect::Process( int iLength, double dSampleRate, int iHostSamplePos, float ** ppData, int iChannelCount )
{
	IAudioProcessor * processor = NULL;
	tresult result = m_pComponent->queryInterface( IAudioProcessor::iid, (void**) &processor );
	ARA_INTERNAL_ASSERT( result == kResultOk );
	ARA_INTERNAL_ASSERT( processor );

	// Zero buffers
	for ( int c = 0; c < iChannelCount; c++ )
		dsp::Zero( ppData[c], iLength );

	AudioBusBuffers inputs;
	inputs.numChannels = 1;
	inputs.silenceFlags = (uint64) (-1);
	inputs.channelBuffers32 = ppData;

	AudioBusBuffers outputs;
	outputs.numChannels = 1;
	outputs.silenceFlags = 0;
	outputs.channelBuffers32 = ppData;

	// in order for Melodyne to render the ARA data, it must be set to playback mode (in stop, its built-in pre-listening logic is active)
	// thus we implement some crude, minimal transport information here.
	ProcessContext context;
	context.state = ProcessContext::kPlaying;
	context.sampleRate = dSampleRate;
	context.projectTimeSamples = iHostSamplePos;

	ProcessData data;
	data.processMode = kRealtime;
	data.symbolicSampleSize = kSample32;
	data.numSamples = iLength;
	data.numInputs = iChannelCount;
	data.numOutputs = iChannelCount;
	data.inputs = &inputs;
	data.outputs = &outputs;
	data.processContext = &context;
	result = processor->process( data );
	ARA_INTERNAL_ASSERT( result == kResultOk );

	processor->release();
}