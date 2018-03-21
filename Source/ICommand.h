#pragma once

// Simple command pattern that I'm using to send data across the ARA / UI thread

#include "../JuceLibraryCode/JuceHeader.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"

struct ICommand
{
	enum class Type
	{
		SetARASample,   // We have a new ARA sample to load
		SampleLoaded,   // The sample has loaded successfully
		RegionCreated,  // A new region has been created from the sample
		ProgramSet,     // All regions are loaded and synth patch is set
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