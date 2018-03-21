#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../UniSampler/Source/Plugin/UniSampler.h"

class JuceSamplerEditor;

class AudioImageGL : public OpenGLAppComponent
{
	//==============================================================================
	void initialise() override;
	void shutdown() override;
	void render() override;
	void paint( Graphics& g ) override;

	void mouseDown( const MouseEvent& e ) override;

	struct Vertex
	{
		float position[2];
	};

	struct Attributes
	{
		//...
		std::unique_ptr<OpenGLShaderProgram::Attribute> position;
		Attributes( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram );
		void enable( OpenGLContext& openGLContext );
		void disable( OpenGLContext& openGLContext );
	};
	struct Uniforms
	{
		//...
		std::unique_ptr<OpenGLShaderProgram::Uniform> color;
		Uniforms ( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram );
	};
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
	// Your private member variables go here...
	String vertexShader;
	String fragmentShader;
	std::unique_ptr<OpenGLShaderProgram> shader;
	std::unique_ptr<Attributes> attributes;
	std::unique_ptr<Uniforms> uniforms;
	std::unique_ptr<VertexBuffer> vertices;
	
	std::mutex m_muGL;
	juce::Colour m_Color;
	UniSampler::Sample * m_pSampleToDraw;

	JuceSamplerEditor * m_pEditor;
	void createWaveFromSample();

public:
	AudioImageGL( JuceSamplerEditor * pEditor );
	~AudioImageGL();
	void SetWave( UniSampler::Sample * pSample );
	void SetColor( Colour c );
};
