# ARAResampler
A JUCE plugin that uses Melodyne to create a multisample instrument from a single sample. 

When pitching samples in a multisampler we walk the source samples at a rate indicative of how much we'd like to change 
the pitch; for example to hear a sample an octave higher than source we must play it twice as fast. However that changes
the duration of the sample, which is often an inevitable drawback of sampling synths. 

However, we can use Melodyne to compensate for this change in duration; in our example where we pitched a sample up an octave, 
all we have to do is use melodyne to stretch the sample to double its duration. That way when we play the pitched sample its
duration matches the source. 

The actual sampler is implemented using the UniSampler subrepo.

## Build Instructions

To build, open the .jucer file using the Projucer (tested with JUCE 5.3.0 on Windows.) The three external dependencies are libsndfile
(to load sample files), the VST3 SDK, and the ARA SDK. 

The Unisampler subrepository contains Visual Studio solutions capable of building libsndfile as well as libFLAC/ogg/vorbis. Once 
built, the .jucer expects to find them under an environment variable called SDK, but you are free to configure this as you like 
(these options can be set in the Visual Studio exporter tab in the Projucer.) 

The VST3 SDK is free to download from https://www.steinberg.net/en/company/developers.html - the SDK path is configured by JUCE
in the Visual Studio exporter tab. JUCE just needs the location of the folder. 

The ARA SDK is not checked in but can be added by
- Downloading the ARA SDK
- Pasting the contents of the ARA_SDK/API folder into Source/ARA
- Pasting the two ARA SDK debug helpers (AraDebug.c and AraDebug.h) found in ARA_SDK/Examples/TestCommon into Source/ARA
