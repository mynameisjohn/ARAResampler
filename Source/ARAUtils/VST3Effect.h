//------------------------------------------------------------------------------
//! \file        vst3loader.h
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

#pragma once

#include "../ARA/ARAInterface.h"

#include "../UniSampler/Source/Plugin/Util.h"

namespace Steinberg
{
	namespace Vst
	{
		class IComponent;
	}
}

//-------------------------------------------------------------------------------------------------------
class VST3Effect
{
#if defined(_MSC_VER)
	HMODULE m_libHandle;
#elif defined(__APPLE__)
	CFBundleRef m_libHandle;
#endif
	Steinberg::Vst::IComponent * m_pComponent;
	Steinberg::Vst::IComponent * GetComponent() const;

	std::string m_strPluginName;

	bool isValid() const;


public:
	VST3Effect();
	~VST3Effect();

	VST3Effect( const VST3Effect& other ) = delete;
	VST3Effect& operator=( const VST3Effect& other ) = delete;

	VST3Effect( VST3Effect&& other );
	VST3Effect& operator=( VST3Effect&& other );

	void Clear();

	class RenderContext
	{
		Steinberg::Vst::IComponent * m_pComponent;
	public:
		~RenderContext();
		RenderContext( VST3Effect * pVST3Effect, bool bStereo );
	};

	bool LoadBinary( std::string strPluginName );
	const ARA::ARAFactory * GetARAFactory();
	const ARA::ARAPlugInExtensionInstance * GetARAPlugInExtension( ARA::ARADocumentControllerRef controllerRef );
	void Process( int iLength, double dSampleRate, int iHostSamplePos, float ** ppData, int iChannelCount );
};
