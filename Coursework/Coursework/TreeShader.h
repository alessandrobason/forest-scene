#pragma once

#include "InstanceShader.h"

struct TreeInstanceType {
	float3 position;
	TreeInstanceType() = default;
	TreeInstanceType(const float3 &p) : position(p) {}
	TreeInstanceType(f32 x, f32 y, f32 z) : position(x, y, z) {}
};

/* TreeShader renders all the trees using instancing in a single
 * drawcall.
 * Every frame, when rendering the TreeShader creates an instance
 * buffer and fills it up with data.
 * During the vertex shader stage, it applies vertex manipulation
 * to every tree's vertex. It creates two rotation matrices (on the
 * x and z axis) based on a sin wave.
 * It applies this matrices to bot the position and the normal,
 * this way the shading will still be correct.
 */
class TreeShader : public InstanceShader {
	struct TreeBufferType {
		float3 windOrigin;
		float windAmplitude;
		float windSpeed;
		float3 padding = { 0, 0, 0 };
	};
public:
	TreeShader(Device *device, HWND hwnd);
	~TreeShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *texture, float4 color, float3 cameraPos, float timePassed, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow, const float3 &windOrigin, f32 windAmplitude, f32 windSpeed);

private:
	void initShader(const wchar_t *vs, const wchar_t *dvs);
	void loadVertexShader(const wchar_t *vs);

	ID3D11Buffer *treeBuffer;
};