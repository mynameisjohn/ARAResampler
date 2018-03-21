# ARAResampler
A JUCE plugin that uses Melodyne to create a multisample instrument from a single sample. 

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
