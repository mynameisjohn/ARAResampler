#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"

struct ICommand
{
	enum class Type
	{
		SetARASample,
		SampleLoaded,
		RegionCreated,
		ProgramSet,
		None
	};
	Type eType;
};
using CmdPtr = std::unique_ptr<ICommand>;

class ICommandListener
{
public:
	virtual void HandleCommand( CmdPtr pCMD ) = 0;
};

struct cmdSetARASample : ICommand
{
	cmdSetARASample()
	{
		eType = Type::SetARASample;
	}
	String strSampleName;
};

struct cmdSampleLoaded : ICommand
{
	cmdSampleLoaded()
	{
		eType = Type::SampleLoaded;
	}
	String strSampleName;
	UniSampler::Sample * pSample;
};

struct cmdRegionCreated : ICommand
{
	cmdRegionCreated()
	{
		eType = Type::RegionCreated;
	}
	String strSampleName;
	int iMIDINote;
};

struct cmdProgramSet : ICommand
{
	cmdProgramSet()
	{
		eType = Type::ProgramSet;
	}
};