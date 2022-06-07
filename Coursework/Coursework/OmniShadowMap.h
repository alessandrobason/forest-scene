#pragma once

#include "types.h"

/* Omni-directional shadow map is used for the point light's shadows
 * the theory behind it is the same as a normal shadowmap, render
 * the scene from the point of view of the light using only depth
 * to a texture and use that data to render shadows later.
 * The main difference is that for points lights we need to render
 * a shadowmap for every "side" around the light. Normally we would
 * need to render the scene to six different shadow maps (one for
 * each side), but we can actually use a Cubemap and a geometry
 * shader to render all the faces at the same time. We can then
 * sample it in the pixel shader using a 3d vector
 */
class OmniShadowMap {
public:
	void init(Device *device, int width, int height);
	~OmniShadowMap();

	void bind(DeviceContext *ctx);
	TextureType *getCubemap() { return cubemap; }

	void setViewMatrix(const mat4 &view, int index) { viewMatrices[index] = view; }
	const mat4 &getViewMatrix(int index) { return viewMatrices[index]; }

private:
	ID3D11DepthStencilView *cubeDSV = nullptr;
	TextureType *cubemap = nullptr;
	D3D11_VIEWPORT viewport;
	ID3D11RenderTargetView *renderTarget = nullptr;
	ID3D11Texture2D *depthTex = nullptr;
	ID3D11Texture2D *cubeTexture = nullptr;
	mat4 viewMatrices[6];
};