#pragma once

#include "DefaultShader.h"

// Simply renders a model without any kind of shading 
// using a diffuse color
// also sets the alpha value to 200 for the bloom effect later 
class MonolithShader : public DefaultShader {
	struct MonolithBufferType {
		float4 color;
	};

public:
	MonolithShader(Device *device, HWND hwnd);
	~MonolithShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, const float4 &color);

private:
	void initShader(const wchar_t *ps);

	ID3D11Buffer *monolithBuffer = nullptr;
};
