/***************************************************************
VST3ARAPlugin class

A C++ class meant to wrap a simple ARA VST3 plugin. The plugin 
will be loaded as a binary from disk and its processor
component can be used to render data. 

***************************************************************/

#pragma once

#include "../ARA/ARAInterface.h"

#include "../UniSampler/Source/Plugin/Util.h"

// Forward declare VST component
namespace Steinberg
{
	namespace Vst
	{
		class IComponent;
	}
}

//-------------------------------------------------------------------------------------------------------
class VST3ARAPlugin
{
	// Platform specific library handles
#if defined(_MSC_VER)
	HMODULE m_libHandle;
#elif defined(__APPLE__)
	CFBundleRef m_libHandle;
#endif

	// Main plugin component
	Steinberg::Vst::IComponent * m_pComponent;
	Steinberg::Vst::IComponent * GetComponent() const;

	// Name of plugin binary
	std::string m_strPluginName;

	// Do we have a plugin loaded?
	bool isValid() const;

public:
	// Lifetime of plugin and resources live with the class instance
	VST3ARAPlugin();
	~VST3ARAPlugin();

	// We don't allow copy constructors because this instance is
	// tied to the lifetime of a plugin instance
	VST3ARAPlugin( const VST3ARAPlugin& other ) = delete;
	VST3ARAPlugin& operator=( const VST3ARAPlugin& other ) = delete;

	// We do allow move constructors, which will transfer 
	// ownership of the plugin from one instance to another
	VST3ARAPlugin( VST3ARAPlugin&& other );
	VST3ARAPlugin& operator=( VST3ARAPlugin&& other );

	// Free all plugin resources
	void Clear();

	// A scoped class that, while alive, allows rendering
	// of audio through the plugin (sets up bus config)
	class RenderContext
	{
		Steinberg::Vst::IComponent * m_pComponent;
	public:
		~RenderContext();
		RenderContext( VST3ARAPlugin * pVST3Effect, bool bStereo );
	};

	// Load the plugin binary from disk (returns success)
	bool LoadBinary( std::string strPluginName );

	// Access to ARA factory
	const ARA::ARAFactory * GetARAFactory();

	// Access to ARA plugin extension
	const ARA::ARAPlugInExtensionInstance * GetARAPlugInExtension( ARA::ARADocumentControllerRef controllerRef );

	// Process audio data with the plugin
	void Process( int iLength, double dSampleRate, int iHostSamplePos, float ** ppData, int iChannelCount );
};
