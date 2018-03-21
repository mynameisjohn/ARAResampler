#include "VST3Effect.h"

// This must be defined while we include
// the VST3 SDK and undefined afterwards
#define INIT_CLASS_IID

#include "../ARA/AraDebug.h"
#include "../ARA/ARAVST3.h"
#include "../ARA/ARAInterface.h"

#include "base/source/fstring.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

// Undefine class IID init
#undef INIT_CLASS_IID

// Declare global plugin context
namespace Steinberg
{
	FUnknown* gStandardPluginContext = 0;
}

// Platform specific library access management
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

using namespace Steinberg;
using namespace Vst;
using namespace ARA;

// Make sure these are null to start
VST3Effect::VST3Effect() :
	m_libHandle( nullptr ),
	m_pComponent( nullptr )
{}

// Transfer ownership of the plugin resources completely
VST3Effect::VST3Effect( VST3Effect&& other )
{
	m_libHandle = other.m_libHandle;
	other.m_libHandle = nullptr;
	m_pComponent = other.m_pComponent;
	other.m_pComponent = nullptr;
	m_strPluginName = other.m_strPluginName;
	other.m_strPluginName.clear();
}

// Transfer ownership of the plugin resources completely
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

// Free any resources
VST3Effect::~VST3Effect() 
{
	Clear();
}

// Free or release whatever we've allocated
void VST3Effect::Clear()
{
	// Tear down plugin component
	if ( m_pComponent )
	{
		tresult result = m_pComponent->terminate();
		m_pComponent->release();
		dbgASSERT( result == kResultOk, "Error terminating component for plugin", m_strPluginName );
		m_pComponent = nullptr;
	}

	// Free library
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

	// Clear name
	m_strPluginName.clear();
}

// Plugin component access
IComponent * VST3Effect::GetComponent() const
{
	return m_pComponent;
}

// ARA factory access
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

// Access to ARA plugin extension
const ARA::ARAPlugInExtensionInstance * VST3Effect::GetARAPlugInExtension( ARA::ARADocumentControllerRef controllerRef )
{
	const ARA::ARAPlugInExtensionInstance * result = NULL;

	// Find the entry point interface
	ARA::IPlugInEntryPoint * entry = NULL;
	if ( (m_pComponent->queryInterface( ARA::IPlugInEntryPoint::iid, (void**) &entry ) == kResultTrue) && entry )
	{
		// Bind it to the document controller here and release
		result = entry->bindToDocumentController( controllerRef );
		entry->release();
	}

	return result;
}


// Do we have a library and component?
bool VST3Effect::isValid() const
{
	return m_libHandle && m_pComponent;
}

// Construct scoped rendering context
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
									// Success! Cache the comonent
									m_pComponent = pPlugComponent;
								}
								else
								{
									// Failure - deactivate output plugin buses
									result = pPlugComponent->activateBus( kAudio, kInput, 0, false );
									result = pPlugComponent->activateBus( kAudio, kOutput, 0, false );
								}
							}
							else
							{
								// Failure - deactivate input plugin buses
								result = pPlugComponent->activateBus( kAudio, kInput, 0, false );
							}
						}
					}
				}

				// Release processor component - for now
				processor->release();
			}
		}
	}
}

// Clean up rendereing context (deactivate buses)
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
		m_pComponent = nullptr;
	}
}

// Load the plugin binary from disk
bool VST3Effect::LoadBinary( std::string strPluginName )
{
	// Return success
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

	// Find factory proc function
	GetFactoryProc entryProc = NULL;
	bool bPluginInit = false;
	if ( dbgASSERT( libHandle, "Unable to load plugin binary", strPluginName ) )
	{
		// Find init module function
#if defined(_MSC_VER)
		InitModuleProc initProc = (InitModuleProc)::GetProcAddress( libHandle, kInitModuleProcName );
		if ( dbgASSERT( initProc, "Unable to get init proc" ) )
		{
			// If we found it, get entry proc
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

	// Call entry proc, if we can
	if ( dbgASSERT( entryProc, "Missing entryProc for plugin", strPluginName ) )
	{
		// If we got the factory, get the factory info
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
		// If we got a library handle, free it now
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

// Process function
void VST3Effect::Process( int iLength, double dSampleRate, int iHostSamplePos, float ** ppData, int iChannelCount )
{
	// Get processor component
	IAudioProcessor * processor = NULL;
	tresult result = m_pComponent->queryInterface( IAudioProcessor::iid, (void**) &processor );
	ARA_INTERNAL_ASSERT( result == kResultOk );
	ARA_INTERNAL_ASSERT( processor );

	// Zero buffers
	for ( int c = 0; c < iChannelCount; c++ )
		dsp::Zero( ppData[c], iLength );

	// Configure input and output buses
	AudioBusBuffers inputs;
	inputs.numChannels = 1;
	inputs.silenceFlags = (uint64) (-1);
	inputs.channelBuffers32 = ppData;

	AudioBusBuffers outputs;
	outputs.numChannels = 1;
	outputs.silenceFlags = 0;
	outputs.channelBuffers32 = ppData;

	// Basic transport info
	ProcessContext context;
	context.state = ProcessContext::kPlaying;
	context.sampleRate = dSampleRate;
	context.projectTimeSamples = iHostSamplePos;

	// Create processdata struct
	ProcessData data;
	data.processMode = kRealtime;
	data.symbolicSampleSize = kSample32;
	data.numSamples = iLength;
	data.numInputs = iChannelCount;
	data.numOutputs = iChannelCount;
	data.inputs = &inputs;
	data.outputs = &outputs;
	data.processContext = &context;

	// Process audio into our buffers
	result = processor->process( data );
	ARA_INTERNAL_ASSERT( result == kResultOk );

	// Release processor component
	processor->release();
}