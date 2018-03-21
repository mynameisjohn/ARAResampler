#include "AudioImageGL.h"
#include "ARAResamplerEditor.h"

// initialise override 
void AudioImageGL::initialise()
{
	// We only care about position and uniform color
	vertexShader =
		"attribute vec2 position;\n"
		"\n"
		"uniform vec4 color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(position.xy, 0, 1);\n"
		"}\n";
	fragmentShader =
		"uniform vec4 color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor = color;\n"
		"}\n";

	// Compile and link shader
	std::unique_ptr<OpenGLShaderProgram> newShader ( new OpenGLShaderProgram ( openGLContext ) ); // [1]
	String statusText;
	if ( newShader->addVertexShader ( OpenGLHelpers::translateVertexShaderToV3 ( vertexShader ) )  // [2]
		 && newShader->addFragmentShader ( OpenGLHelpers::translateFragmentShaderToV3 ( fragmentShader ) )
		 && newShader->link() )
	{
		attributes.reset();
		uniforms.reset();
		shader.reset ( newShader.release() );                                                   // [3]
		shader->use();
		attributes.reset ( new Attributes ( openGLContext, *shader ) );
		uniforms.reset ( new Uniforms ( openGLContext, *shader ) );
		statusText = "GLSL: v" + String ( OpenGLShaderProgram::getLanguageVersion(), 2 );
	}
	else
	{
		statusText = newShader->getLastError();                                               // [4]
	}
	
	// set default color to red
	m_Color = Colours::red;
}

// Public access to drawn waveform
// If the GL thread sees m_pSampleToDraw is not null
// it will recrate its geometry to match the sample
void AudioImageGL::SetWave( UniSampler::Sample * pSample )
{
	std::lock_guard<std::mutex> gl( m_muGL );
	m_pSampleToDraw = pSample;
}

// Set color of drawn waveform
void AudioImageGL::SetColor( Colour c )
{
	std::lock_guard<std::mutex> gl( m_muGL );
	m_Color = c;
}

// Create a waveform drawing from an audio sample
void AudioImageGL::createWaveFromSample( UniSampler::Sample * pSample, int nLines /*= 2000*/ )
{
	if ( pSample == nullptr )
		return;

	// Create geometry
	std::vector<Vertex> vPositionData;
	if ( pSample->bStereo )
	{
		
		// 
		for ( int c = 0; c < 2; c++ )
		{
			for ( int i = 0; i < nLines; i++ )
			{
				// Find x position relative to how many lines we're drawing
				float x = dsp::Remap<float>( i, 0, nLines, -1, 1 );

				// Sample level is Y pos, use line index to find corresponding sample index
				int ixSample = dsp::Remap<double>( i, 0, nLines, 0, pSample->size() );
				float y = pSample->at( 0, ixSample );

				// Change short to normalized float if necessary
				if ( !pSample->bFloat )
					y /= SHRT2FLT;

				// For stereo samples draw each channel between [-1, 0] and [0, 1]
				y = dsp::Remap<float>( y, -1, 1, -1 + c, c );
				vPositionData.push_back( { x, -0.5f + c } );
				vPositionData.push_back( { x, y } );
			}
		}
	}
	else
	{
		
		for ( int i = 0; i < nLines; i++ )
		{
			// Find x position relative to how many lines we're drawing
			float x = dsp::Remap<float>( i, 0, nLines, -1, 1 );

			// Sample level is y pos, use line index to find corresponding sample index
			int ixSample = dsp::Remap<double>( i, 0, nLines, 0, pSample->size() );
			float y = pSample->at( 0, ixSample );

			// Change short to normalized float if necessary
			if ( !pSample->bFloat )
				y /= SHRT2FLT;

			vPositionData.push_back( { x, 0 } );
			vPositionData.push_back( { x, y } );
		}
	}

	// Reconstruct VBO (this will erase the old VBO)
	vertices.reset( new VertexBuffer( openGLContext, vPositionData ) );
}

// The waveform area represents our "open sample" button
void AudioImageGL::mouseDown( const MouseEvent& e )
{
	FileChooser chooser( "Open an Sample File...",
						 File::nonexistent,
						 "*.wav;*.ogg;*.flac" );
	if ( chooser.browseForFileToOpen() )
	{
		// Kick off sample loading
		File file( chooser.getResult() );
		m_pEditor->OnNewWave( file );
	}
}

AudioImageGL::AudioImageGL( ARAResamplerEditor * pEditor ) :
	m_pEditor( pEditor )
	, m_pSampleToDraw( nullptr )
{}

// Shutdown and shutdown openGL
AudioImageGL::~AudioImageGL()
{
	shutdown();
	shutdownOpenGL();
}

// Reset any GL members
void AudioImageGL::shutdown()
{
	// Free any GL objects created for rendering here.
	shader.reset();
	attributes.reset();
	uniforms.reset();
}

// Update the OpenGL data
void AudioImageGL::render()
{
	// Potentially updated variables
	Colour colorToUse;
	UniSampler::Sample * pSampleToDraw = nullptr;
	{
		// Keep this locked while we look for a new sample or color
		std::lock_guard<std::mutex> lg( m_muGL );

		// Cache these while mutex is locked
		// Always null out m_pSampleToDraw so we 
		// don't redraw it unnecessarily
		colorToUse = m_Color;
		pSampleToDraw = m_pSampleToDraw;
		m_pSampleToDraw = nullptr;
	}

	// Maybe redraw sample
	if ( pSampleToDraw )
	{
		createWaveFromSample( pSampleToDraw );
	}

	// If we have no vertex data then get out
	if ( vertices == nullptr )
		return;

	// This clears the context with a black background.
	OpenGLHelpers::clear ( Colours::black );

	// Bind shader
	jassert ( OpenGLHelpers::isContextActive() );
	float desktopScale = (float) openGLContext.getRenderingScale(); // [1]
	OpenGLHelpers::clear ( getLookAndFeel().findColour ( ResizableWindow::backgroundColourId ) ); // [2]
	glEnable ( GL_BLEND );                                           // [3]
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glViewport ( 0, 0, roundToInt ( desktopScale * getWidth() ), roundToInt ( desktopScale * getHeight() ) ); // [4]
	shader->use();

	// Set color
	uniforms->color->set( m_Color.getFloatRed(), m_Color.getFloatGreen(), m_Color.getFloatBlue(), m_Color.getFloatAlpha() );

	// Draw geometry
	vertices->bind();
	attributes->enable( openGLContext );
	glDrawElements( GL_LINES, vertices->nIndices, GL_UNSIGNED_INT, 0 );
	attributes->disable( openGLContext );

	// Clear bound buffers
	openGLContext.extensions.glBindBuffer ( GL_ARRAY_BUFFER, 0 );    // [9]
	openGLContext.extensions.glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

// This must be implemented, but we don't really need it
void AudioImageGL::paint( Graphics& g )
{

}

// Our attributes constructor simply creates an attribute called position, which should exist in the shader
AudioImageGL::Attributes::Attributes( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram )
{
	String strAttrName = "position";
	int iAttrLoc = openGLContext.extensions.glGetAttribLocation( shaderProgram.getProgramID(), strAttrName.toRawUTF8() );
	if ( iAttrLoc >= 0 )
		position.reset( new OpenGLShaderProgram::Attribute( shaderProgram, strAttrName.toRawUTF8() ) );
}

// Configure the position attribute as a 2D float
// (actual array data is uploaded in the VertexBuffer constructor)
void AudioImageGL::Attributes::enable( OpenGLContext& openGLContext )
{
	if ( position )
	{
		openGLContext.extensions.glVertexAttribPointer( position->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), 0 );
		openGLContext.extensions.glEnableVertexAttribArray( position->attributeID );
	}
}

// Disable position array 
void AudioImageGL::Attributes::disable( OpenGLContext& openGLContext )
{
	if ( position )
	{
		openGLContext.extensions.glDisableVertexAttribArray( position->attributeID );
	}
}

// Our uniform constructor finds the color uniform variable in the shader
AudioImageGL::Uniforms::Uniforms( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram )
{
	String strUniName = "color";
	int iUniLoc = openGLContext.extensions.glGetUniformLocation( shaderProgram.getProgramID(), strUniName.toRawUTF8() );
	if ( iUniLoc >= 0 )
		color.reset( new OpenGLShaderProgram::Uniform( shaderProgram, strUniName.toRawUTF8() ) );
}

// VertexBuffer constructor creates position and index data from a collection of 2D floats
AudioImageGL::VertexBuffer::VertexBuffer( OpenGLContext& context, const std::vector<Vertex>&vPositionData ) :
	openGLContext( context )
	, VBO( 0 )
	, IBO( 0 )
	, nIndices( 0 )
{
	// Create and upload position data
	openGLContext.extensions.glGenBuffers( 1, &VBO );
	openGLContext.extensions.glBindBuffer( GL_ARRAY_BUFFER, VBO );
	openGLContext.extensions.glBufferData( GL_ARRAY_BUFFER, vPositionData.size() * sizeof( Vertex ), vPositionData.data(), GL_STATIC_DRAW );

	// Find indices - we're drawing lines so this is just 
	// a series of ordered pairs from beginning to end
	nIndices = vPositionData.size();
	std::vector<GLuint> vIndices( nIndices );
	for ( GLuint i = 0; i < nIndices; i++ )
		vIndices[i] = i;

	// Upload index data
	openGLContext.extensions.glGenBuffers( 1, &IBO );
	openGLContext.extensions.glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );
	openGLContext.extensions.glBufferData( GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof( GLuint ), vIndices.data(), GL_STATIC_DRAW );
}

// Delete our position and index data
AudioImageGL::VertexBuffer::~VertexBuffer()
{
	openGLContext.extensions.glDeleteBuffers ( 1, &VBO );
	openGLContext.extensions.glDeleteBuffers ( 1, &IBO );
}

// Assign vertex position and index to their corresponding GL locations
void AudioImageGL::VertexBuffer::bind()
{
	openGLContext.extensions.glBindBuffer ( GL_ARRAY_BUFFER, VBO );
	openGLContext.extensions.glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, IBO );
}