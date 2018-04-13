#pragma once
#include <graphics_framework.h>

class GBuffer
{
public:
	enum GBUFFER_TEXTURE_TYPE {
		GBUFFER_TEXTURE_TYPE_POSITION,
		GBUFFER_TEXTURE_TYPE_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_NORMAL,
		GBUFFER_TEXTURE_TYPE_TEXCOORD,
		NUM_TEXTURES
	};

	GBuffer();
	~GBuffer();

	bool Init(unsigned int screenWidth, unsigned int screenHeight);

	void BindForReading();
	void BindForWriting();
	void SetReadBuffer(GBUFFER_TEXTURE_TYPE textureType);

	GLuint GetTexture(GBUFFER_TEXTURE_TYPE textureType);
	GLuint GetDepthTexture();

private:

	GLuint m_fbo;
	GLuint m_textures[NUM_TEXTURES];
	GLuint m_depthTexture;
};

