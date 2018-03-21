/******************************************************************************

	ARAResamplerProcessor.h

This class is essentially the basic JUCE processor that 
additionally spawns a thread meant to manage an ARA plugin. 
It communicates with the editor class to use the plugin 
to generate a multisample synth patch from a single sample

******************************************************************************/
#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include "../JuceLibraryCode/JuceHeader.h"

#include "ICommand.h"

// Forward declare sampler and editor class
class UniSampler;
class ARAResamplerEditor;

//==============================================================================
/**
*/
class ARAResamplerProcessor  : public AudioProcessor
	, public ICommandListener
{
	// Pointer to sampler and static bool indicating
	// that we need to initialize static sampler members
	static std::atomic_bool s_bInitSamplerStatic;
	std::unique_ptr<UniSampler> m_pSampler;

	// Thread safe access to the sample we'll
	// be stretching and using with the ARA plugin
	String m_strSampleToLoad;
	std::mutex m_muLoadSample;
public:
    //==============================================================================
    ARAResamplerProcessor();
	~ARAResamplerProcessor();

	// Access to UniSampler instance
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
	// These are not implemented - there isn't much of a patch to store
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	// Command handler
	void HandleCommand( CmdPtr pCMD ) override;

	// ARA thread and lifetime management
	std::atomic_bool m_abARAThreadRun;
	std::thread m_ARAThread;

	void OnEditorDestroyed();

private:
	// Editor pointer and internal transport position
	ARAResamplerEditor * m_pEditor;
	float m_fHostPosInternal;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ARAResamplerProcessor)
};
