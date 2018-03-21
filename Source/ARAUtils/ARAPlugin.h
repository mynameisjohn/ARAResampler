#pragma once

#include <string>
#include <memory>

#include "../ARA/ARAInterface.h"

#include "VST3Effect.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"
#include "../ICommand.h"

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

class ARAPlugin
{
	std::string m_strPluginName;
	VST3Effect m_vst3Effect;
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