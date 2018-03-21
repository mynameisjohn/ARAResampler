#pragma once

/***************************************************************
ARAPlugin class

Simple C++ class meant to wrap an ARA plugin (Melodyne, specifically)
and create a multisample instrument from a single sample. This works
by taking the source sample and stretching / squashing it such that
when it is repitched to the desired note its dureation matches the
source sample (meaning higher pitches sustain and lower pitches
don't drag on forever). 

***************************************************************/

#include <string>
#include <memory>

#include "../ARA/ARAInterface.h"

#include "VST3ARAPlugin.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "../ICommand.h"

class ARAResampler
{
	// Name of ARA plugin (must be a VST3)
	std::string m_strPluginName;

	// Our VST3 Accessor to the plugin
	VST3ARAPlugin m_vst3Plugin;

	// Pointer to sampler instance we'll be dealing with
	UniSampler * m_pSampler;

	// Current root sample; points to a sample in the global
	// sample pool, so it may be invalidated (keep null as often as possible)
	UniSampler::Sample * m_pCurrentRootSample;

	// Scoped pointer to ARA document controller and properties
	std::unique_ptr<ARA::ARADocumentControllerHostInstance> m_upDocController;
	std::unique_ptr<ARA::ARADocumentProperties > m_upDocProperties;

	// Doc controller ref - a member of the doc controller
	ARA::ARADocumentControllerRef m_pDocControllerRef;

	// ARA Plugin access
	ARA::ARAInterfaceConfiguration m_InterfaceConfig;
	const ARA::ARADocumentControllerInterface * m_pDocControllerInterface;
	ARA::ARAPlugInExtensionRef m_pPluginRef;
	const ARA::ARAPlugInExtensionInterface * m_pPluginExtensionInterface;
	const ARA::ARAFactory * m_pARAFactory;
	void clear();

public:
	ARAResampler( UniSampler * pSampler );
	~ARAResampler();

	// Load an ARA plugin from disk
	bool LoadPlugin( std::string strPluginName );

	// Create a multisample synth patch from a single root sample
	// Notify command listener of progress
	bool CreateSamples( std::string strRootSample, ICommandListener * pCMDListener = nullptr );

	// Access to current root sample - only valid during scope of CreateSamples
	UniSampler::Sample * GetCurrentRootSample() const;
};