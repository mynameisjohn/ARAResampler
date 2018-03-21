/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include "../JuceLibraryCode/JuceHeader.h"

#include "ICommand.h"

class ARAResamplerEditor;
class UniSampler;

//==============================================================================
/**
*/
class ARAResamplerProcessor  : public AudioProcessor
	, public ICommandListener
{
	static std::atomic_bool s_bInitStaticFX;
	std::unique_ptr<UniSampler> m_pSampler;

	String m_strSampleToLoad;
	std::mutex m_muLoadSample;
public:
    //==============================================================================
    ARAResamplerProcessor();
	~ARAResamplerProcessor();

	UniSampler * GetSampler() const;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	bool SetRootPathString(String strDefaultPath);

	void HandleCommand( CmdPtr pCMD ) override;

	std::atomic_bool m_abARAThreadRun;
	std::thread m_ARAThread;

	String GetCurrentFile();
	String GetRootPath();
	void OnEditorDestroyed();

private:
	MemoryBlock m_mbProgramText;
	MemoryBlock m_mbRootPathText;
	ARAResamplerEditor * m_pEditor;
	float m_fHostPosInternal;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ARAResamplerProcessor)
};
