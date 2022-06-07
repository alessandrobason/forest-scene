#pragma once

#include "DefaultShader.h"

struct BlurBufferType {
	float bloomAmount;
	float3 padding;
};

/* Bloom shader takes care of selecting only the pixels that need to be
 * "bloomed". Every monolith pixel has an alpha value higher than 1.5.
 */
class BloomShader : public DefaultShader {
public:
	BloomShader(Device *device, HWND hwnd);

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture);

private:
	void initShader(const wchar_t *ps);
};

class BlurHorShader : public DefaultShader {
public:
	BlurHorShader(Device *device, HWND hwnd);
	~BlurHorShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture, f32 bloomAmount);

private:
	void initShader(const wchar_t *ps);

	ID3D11Buffer *blurBuffer = nullptr;
};

class BlurVerShader : public DefaultShader {
public:
	BlurVerShader(Device *device, HWND hwnd);
	~BlurVerShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture, f32 bloomAmount);

private:
	void initShader(const wchar_t *ps);

	ID3D11Buffer *blurBuffer = nullptr;
};

class MixShader : public DefaultShader {
public:
	MixShader(Device *device, HWND hwnd);

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *textureA, TextureType *textureB);

private:
	void initShader(const wchar_t *ps);
};

class Bloom {
public:
	void init(Device *device, DeviceContext *deviceContext, int screenWidth, int screenHeight, HWND hwnd);
	~Bloom();

	TextureType *apply(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture);
	void gui();

private:
	BloomShader    *bloomShader    = nullptr;
	BlurHorShader  *blurHorShader  = nullptr;
	BlurVerShader  *blurVerShader  = nullptr;
	MixShader      *mixShader      = nullptr;

	RenderTexture *bloomTarget   = nullptr;
	RenderTexture *blurHorTarget = nullptr;
	RenderTexture *blurVerTarget = nullptr;
	RenderTexture *mixTarget     = nullptr;

	MMesh mesh;

	f32 bloomAmount = 1.f;
};
