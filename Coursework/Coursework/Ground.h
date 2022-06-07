#pragma once

#include "DefaultShader.h"
#include "GrassShader.h"
#include "vec.h"

// Very similar to a plane, but lets you also choose the size of the plane itself without needing
// to modify the world matrix later
class GroundMesh : public MMesh {
public:
	void init(Device *device, DeviceContext *ctx, vec2i size = { 200, 200 }, vec2i resolution = { 10, 10 });
};

/* Tesselates the ground dynamically depending on the distance to the camera
 * this is used for the grass, this way it doesn't render all the grass far 
 * away.
 * To achieve this we have a maximum factor all around the camera until a
 * cutoff distance, after this the factor linearly decreases until
 * max distance, where the factor will only be one.
 * Because the grass is generated in the geometry shader using pseudo-random
 * number that change depending on its position, the grass appears to
 * change a lot. If this happens close to the camera it will be very noticeable.
 * Using a cutoff distance we can ensure that the player doesn't see this changes
 */
class GroundShader : public DefaultShader {
	struct GroundBufferType {
		float3 cameraPos;
		float cutoffDist;
		float maxDist;
		float maxFactor;
		float2 padding = { 0, 0 };
	};

public:
	GroundShader(Device *device, HWND hwnd);
	~GroundShader();

	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *texture, float4 color, float3 cameraPos, f32 timePassed, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow, f32 cutoffDistance, f32 maxDist, f32 maxFactor);
	void render(DeviceContext *ctx, MMesh &mesh);

private:
	void initShader(const wchar_t *vs, const wchar_t *hs, const wchar_t *ds, const wchar_t *dvs);

	ID3D11Buffer *groundBuffer = nullptr;
};

class Ground {
public:
	void init(D3D *ctx, HWND hwnd, TextureIdManager &tmanager);
	~Ground();

	void update(f32 dt);
	void renderGrass(DeviceContext *ctx, const mat4 &view, const mat4 &proj, const float3 &camPos, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow);
	void renderGround(DeviceContext *ctx, const mat4 &view, const mat4 &proj, const float3 &camPos, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow);

	void gui(bool &open);

	GrassShader *getGrassShader() { return grassShader; }
	GroundShader *getGroundShader() { return groundShader; }

	void setWindOrigin(const float3 &origin);

private:
	GroundShader *groundShader = nullptr;
	GrassShader *grassShader = nullptr;

	WindData windData;
	f32 cutoffDistance = 70.f;
	f32 maxDistance = 220.f;
	f32 maxFactor   = 25.f;
	bool shouldDrawGrass = true;

	mat4 groundMatrix = XMMatrixIdentity();
	GroundMesh ground;

	TextureIdManager *tmanager = nullptr;
	int grassTextureId = -1;
	int grassPatternId = -1;
	int groundTextureId = -1;
};