#pragma once

#include "DefaultShader.h"

struct SkyData {
	float3 lightDir         = float3(0.f, 0.f, 0.f);
	float  padding          = 0.f;
	float4 sunCol           = float4(1.000f, 0.529f, 0.000f, 1.0);
	float4 moonCol          = float4(1.000f, 0.862f, 0.529f, 1.0);
	float4 dayBottomCol     = float4(0.230f, 0.730f, 0.580f, 1.f);
	float4 dayTopCol        = float4(0.510f, 0.620f, 1.000f, 1.f);
	float4 nightBottomCol   = float4(0.043f, 0.043f, 0.200f, 1.f);
	float4 nightTopCol      = float4(0.086f, 0.004f, 0.004f, 1.f);
	float4 sunsetCol        = float4(0.775f, 0.159f, 0.000f, 1.f);
	float  radius           = 0.200f;
	float  moonOffset       = 0.141f;
	float  horizonIntensity = 2.456f;
	float  starsExponent    = 11.555f;
};

/* Render a sky in the skybox, everthing apart from what is in
 * the SkyData structure is dynamically calculated every frame.
 * It renders a circle for the sun, a circle for the moon with
 * another circle inside to give it a more stylized look.
 * The sun color gets less intense over the top, this makes it
 * look like the sun gets red when getting closer to the horizon.
 * The sky color is a mix of the day color (a mix itself of
 * the dayBottom and dayTop color depending on the y value) and
 * a night color (same thing as the day color). The mix between
 * day and night color is decided using the light y position.
 * When the sunlight gets close to the horizon the sky is given
 * a tint based on sunsetColor.
 * During the night a stars texture is shown, the texture color
 * uses a stars exponent to get only the brighters stars and hide
 * the background color. The texture is also moved to simulate the
 * rotation of the earth
 */
class SkyShader : public DefaultShader {
public:
	SkyShader(Device *device, HWND hwnd);
	~SkyShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *texture, float3 cameraPos, float timePassed, Light *light, const SkyData &skyData);

private:
	void initShader(const wchar_t *vs, const wchar_t *ps);

	ID3D11Buffer *skyBuffer = nullptr;
};
