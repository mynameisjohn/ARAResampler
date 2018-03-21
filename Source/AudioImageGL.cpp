#include "AudioImageGL.h"
#include "PluginEditor.h"

void AudioImageGL::initialise()
{
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

	m_Color = Colours::red;
}

void AudioImageGL::SetWave( UniSampler::Sample * pSample )
{
	std::lock_guard<std::mutex> gl( m_muGL );
	m_pSampleToDraw = pSample;
}

void AudioImageGL::SetColor( Colour c )
{
	std::lock_guard<std::mutex> gl( m_muGL );
	m_Color = c;
}

void AudioImageGL::createWaveFromSample()
{
	if ( m_pSampleToDraw == nullptr )
		return;

	// Create geometry
	int nPoints = 1000;
	std::vector<Vertex> vPositionData;
	if ( m_pSampleToDraw->bStereo )
	{
		for ( int c = 0; c < 2; c++ )
		{
			for ( int i = 0; i < nPoints; i++ )
			{
				float x = dsp::Remap<float>( i, 0, nPoints, -1, 1 );
				int ixSample = dsp::Remap<double>( i, 0, nPoints, 0, m_pSampleToDraw->size() );
				float y = m_pSampleToDraw->at( 0, ixSample );
				if ( !m_pSampleToDraw->bFloat )
					y /= SHRT2FLT;
				y = dsp::Remap<float>( y, -1, 1, -1 + c, c );
				vPositionData.push_back( { x, -0.5f + c } );
				vPositionData.push_back( { x, y } );
			}
		}
	}
	else
	{
		for ( int i = 0; i < nPoints; i++ )
		{
			float x = dsp::Remap<float>( i, 0, nPoints, -1, 1 );
			int ixSample = dsp::Remap<double>( i, 0, nPoints, 0, m_pSampleToDraw->size() );
			float y = m_pSampleToDraw->at( 0, ixSample );
			if ( !m_pSampleToDraw->bFloat )
				y /= SHRT2FLT;
			vPositionData.push_back( { x, 0 } );
			vPositionData.push_back( { x, y } );
		}
	}

	vertices.reset( new VertexBuffer( openGLContext, vPositionData ) );

	m_pSampleToDraw = nullptr;
}

void AudioImageGL::mouseDown( const MouseEvent& e )
{
	// This is our "open sample" button
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

AudioImageGL::AudioImageGL( JuceSamplerEditor * pEditor ) :
	m_pEditor( pEditor )
	, m_pSampleToDraw( nullptr )
{

}

AudioImageGL::~AudioImageGL()
{
	shutdown();
	shutdownOpenGL();
}

void AudioImageGL::shutdown()
{
	// Free any GL objects created for rendering here.
	shader.reset();
	attributes.reset();
	uniforms.reset();
}

void AudioImageGL::render()
{
	std::lock_guard<std::mutex> lg( m_muGL );

	if ( m_pSampleToDraw )
	{
		createWaveFromSample();
	}

	if ( vertices == nullptr )
		return;

	// This clears the context with a black background.
	OpenGLHelpers::clear ( Colours::black );

	// Add your rendering code here...    
	jassert ( OpenGLHelpers::isContextActive() );
	auto desktopScale = (float) openGLContext.getRenderingScale(); // [1]
	OpenGLHelpers::clear ( getLookAndFeel().findColour ( ResizableWindow::backgroundColourId ) ); // [2]
	glEnable ( GL_BLEND );                                           // [3]
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glViewport ( 0, 0, roundToInt ( desktopScale * getWidth() ), roundToInt ( desktopScale * getHeight() ) ); // [4]
	shader->use();

	// Set color
	uniforms->color->set( m_Color.getFloatRed(), m_Color.getFloatGreen(), m_Color.getFloatBlue(), m_Color.getFloatAlpha() );

	vertices->bind();
	attributes->enable( openGLContext );
	glDrawElements( GL_LINES, vertices->nIndices, GL_UNSIGNED_INT, 0 );
	attributes->disable( openGLContext );

	openGLContext.extensions.glBindBuffer ( GL_ARRAY_BUFFER, 0 );    // [9]
	openGLContext.extensions.glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void AudioImageGL::paint( Graphics& g )
{

}

AudioImageGL::Attributes::Attributes( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram )
{
	String strAttrName = "position";
	int iAttrLoc = openGLContext.extensions.glGetAttribLocation( shaderProgram.getProgramID(), strAttrName.toRawUTF8() );
	if ( iAttrLoc >= 0 )
		position.reset( new OpenGLShaderProgram::Attribute( shaderProgram, strAttrName.toRawUTF8() ) );
}

void AudioImageGL::Attributes::enable( OpenGLContext& openGLContext )
{
	if ( position )
	{
		openGLContext.extensions.glVertexAttribPointer( position->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), 0 );
		openGLContext.extensions.glEnableVertexAttribArray( position->attributeID );
	}
}

void AudioImageGL::Attributes::disable( OpenGLContext& openGLContext )
{
	if ( position )
	{
		openGLContext.extensions.glDisableVertexAttribArray( position->attributeID );
	}
}

AudioImageGL::Uniforms::Uniforms( OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram )
{
	String strUniName = "color";
	int iUniLoc = openGLContext.extensions.glGetUniformLocation( shaderProgram.getProgramID(), strUniName.toRawUTF8() );
	if ( iUniLoc >= 0 )
		color.reset( new OpenGLShaderProgram::Uniform( shaderProgram, strUniName.toRawUTF8() ) );
}

AudioImageGL::VertexBuffer::VertexBuffer( OpenGLContext& context, const std::vector<Vertex>&vPositionData ) :
	openGLContext( context )
	, VBO( 0 )
	, IBO( 0 )
	, nIndices( 0 )
{
	openGLContext.extensions.glGenBuffers( 1, &VBO );
	openGLContext.extensions.glBindBuffer( GL_ARRAY_BUFFER, VBO );
	openGLContext.extensions.glBufferData( GL_ARRAY_BUFFER, vPositionData.size() * sizeof( Vertex ), vPositionData.data(), GL_STATIC_DRAW );

	nIndices = vPositionData.size();
	std::vector<GLuint> vIndices( nIndices );
	for ( GLuint i = 0; i < nIndices; i++ )
		vIndices[i] = i;

	openGLContext.extensions.glGenBuffers( 1, &IBO );
	openGLContext.extensions.glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );
	openGLContext.extensions.glBufferData( GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof( GLuint ), vIndices.data(), GL_STATIC_DRAW );
}

AudioImageGL::VertexBuffer::~VertexBuffer()
{
	openGLContext.extensions.glDeleteBuffers ( 1, &VBO );
	openGLContext.extensions.glDeleteBuffers ( 1, &IBO );
}

void AudioImageGL::VertexBuffer::bind()
{
	openGLContext.extensions.glBindBuffer ( GL_ARRAY_BUFFER, VBO );
	openGLContext.extensions.glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, IBO );
}