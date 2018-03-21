#pragma once

#include <string>
#include <memory>

#include "../ARA/ARAInterface.h"

#include "VST3ARAPlugin.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "../ICommand.h"

class ARAPlugin
{
	std::string m_strPluginName;
	VST3ARAPlugin m_vst3Effect;
	UniSampler * m_pSampler;
	UniSampler::Sample * m_pCurrentRootSample;
	std::unique_ptr<ARA::ARADocumentControllerHostInstance> m_upDocController;
	std::unique_ptr<ARA::ARADocumentProperties > m_upDocProperties;

	ARA::ARAInterfaceConfiguration m_InterfaceConfig;
	const ARA::ARADocumentControllerInterface * m_pDocControllerInterface;
	ARA::ARADocumentControllerRef m_pDocControllerRef;
	ARA::ARAPlugInExtensionRef m_pPluginRef;
	const ARA::ARAPlugInExtensionInterface * m_pPluginExtensionInterface;
	const ARA::ARAFactory * m_pARAFactory;
	void clear();

public:
	ARAPlugin( UniSampler * pSampler );
	~ARAPlugin();
	bool LoadPlugin( std::string strPluginName );
	bool CreateSamples( std::string strRootSample, ICommandListener * pCMDListener = nullptr );
	UniSampler::Sample * GetCurrentRootSample() const;
};