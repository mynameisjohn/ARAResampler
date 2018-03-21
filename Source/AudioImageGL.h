/***************************************************************
	AudioImageGL class

A JUCE OpenGL component that we use to render a sample
waveform using OpenGL lines. A lot of this design was
taken from the JUCE OpenGL example 
https://docs.juce.com/master/tutorial_open_gl_application.html

***************************************************************/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"

// Forward declare our editor class
class ARAResamplerEditor;

class AudioImageGL : public OpenGLAppComponent
{
	//==============================================================================
	void initialise() override;
	void shutdown() override;
	void render() override;
	void paint( Graphics& g ) override;

	void mouseDown( const MouseEvent& e ) override;

	// 2D Vertex struct
	struct Vertex
	{
		float position[2];
	};

	// Our per-vertex attributes - all we care about is position
	struct Attributes
	{
		//...
		std::unique_ptr<OpenGLShaderProgram::Attribute> position;
		Attributes( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram );
		void enable( OpenGLContext& openGLContext );
		void disable( OpenGLContext& openGLContext );
	};

	// Uniforms - the only one we use is color
	struct Uniforms
	{
		//...
		std::unique_ptr<OpenGLShaderProgram::Uniform> color;
		Uniforms ( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram );
	};

	// Our VBO object - just position and index buffers are needed
	struct VertexBuffer
	{
		GLuint VBO, IBO;
		GLuint nIndices;
		OpenGLContext& openGLContext;
		VertexBuffer( OpenGLContext& context, const std::vector<Vertex>& vPositionData );
		~VertexBuffer();
		void bind();
	};

	//==============================================================================
	// Shader program and variables
	String vertexShader;
	String fragmentShader;
	std::unique_ptr<OpenGLShaderProgram> shader;
	std::unique_ptr<Attributes> attributes;
	std::unique_ptr<Uniforms> uniforms;

	// VBO
	std::unique_ptr<VertexBuffer> vertices;
	
	// The color we use to render
	juce::Colour m_Color;

	// Our drawn sample - if this is not null 
	// then we refresh the image and null it
	UniSampler::Sample * m_pSampleToDraw;

	// Protects access to the OpenGL thread
	std::mutex m_muGL;

	// Editor pointer
	ARAResamplerEditor * m_pEditor;

	// Function to create wave geometry from sample
	// The number of lines controls the resolution of the wave
	void createWaveFromSample( UniSampler::Sample * pSample, int nPoints = 2000 );

public:
	AudioImageGL( ARAResamplerEditor * pEditor );
	~AudioImageGL();

	// Public access to wave and color
	// These lock the GL mutex
	void SetWave( UniSampler::Sample * pSample );
	void SetColor( Colour c );
};
