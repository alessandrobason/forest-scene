#pragma once

#include "DefaultShader.h"
#include "TextureIdManager.h"

struct WindData {
	float timePassed;
	float3 windOrigin;
	float windSpeed;
	float waveAmplitude;
};

/* Grass shader uses a geometry shader to create grass geometry 
 * dynamically at runtime.
 * For every triangle in a plane, it checks if it's pixel on the 
 * grass pattern texture. When a pixel red value is zero the 
 * grass is not rendered, otherwise it is.
 * The shader uses pseudo random number generation to make every
 * grass' height, rotation and color slightly randomized.
 */
class GrassShader : public DefaultShader {
	struct GrassBufferType {
		float timePassed;
		float3 windOrigin;
		float windSpeed;
		float waveAmplitude;
		float2 padding;
		mat4 spotLightMVP;
	};

	struct GroundBufferType {
		float3 cameraPos;
		float cutoffDist;
		float maxDist;
		float maxFactor;
		float2 padding = { 0, 0 };
	};

public:
	GrassShader(Device *device, HWND hwnd);
	~GrassShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *grassPattern, TextureType *grassTexture, const WindData &windData, const float3 &camPos, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow, f32 cutoffDist, f32 maxDist, f32 maxFactor);
	void render(DeviceContext *ctx, MMesh &mesh);

private:
	void initShader(const wchar_t *vs, const wchar_t *hs, const wchar_t *ds, const wchar_t *gs, const wchar_t *ps);

	ID3D11Buffer *grassBuffer = nullptr;
	ID3D11Buffer *groundBuffer = nullptr;
};
