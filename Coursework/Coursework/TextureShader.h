#pragma once

#include "DefaultShader.h"

class TextureShader : public DefaultShader {
public:
	TextureShader(Device *device, HWND hwnd);

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *texture);

private:
	void initShader(const wchar_t *ps);
};