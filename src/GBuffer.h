#pragma once
#include <graphics_framework.h>

class GBuffer
{
public:
	enum GBUFFER_TEXTURE_TYPE {
		GBUFFER_TEXTURE_TYPE_POSITION,
		GBUFFER_TEXTURE_TYPE_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_NORMAL,
		GBUFFER_TEXTURE_TYPE_MAT_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_MAT_SPECULAR,
		NUM_TEXTURES
	};

	GBuffer();
	~GBuffer();

	bool Init(unsigned int screenWidth, unsigned int screenHeight);

	void StartFrame();
	void BindForGeometryPass();
	void BindForStencilPass();
	void BindForLightPass();
	void BindForFinalPass();

	void SetReadBuffer(GBUFFER_TEXTURE_TYPE textureType);

	GLuint GetTexture(GBUFFER_TEXTURE_TYPE textureType);
	GLuint GetFinalTexture();
	GLuint GetDepthTexture();

private:

	GLuint m_fbo;
	GLuint m_textures[NUM_TEXTURES];
	GLuint m_depthTexture;
	GLuint m_finalTexture;
};

