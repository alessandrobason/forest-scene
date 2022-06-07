#pragma once

#include "SkyShader.h"

// SkyMesh is just a sphere mesh with better uvs used for the sky box
struct SkyMesh : public MMesh {
	void init(Device *device, DeviceContext *ctx, int resolution = 20);
};

/* The sky class manages everything that regards the skybox, 
 * from creating and rendering the skybox mesh to updating the
 * sunlight position.
 * The gui lets you print to console to easily copy-paste the
 * sky data in the SkyShader
 */
class Sky {
public:
	void init(Device *device, DeviceContext *ctx, HWND hwnd, TextureIdManager *textureManager, Light *dirlightPtr);
	~Sky();

	void update(float dt);
	void render(D3D *renderer, FPCamera *camera);
	void gui(bool &open);

private:
	void writeToConsole();

	SkyShader *shader = nullptr;
	TextureIdManager *tmanager = nullptr;
	Light *sunlight = nullptr;
	SkyMesh mesh;

	SkyData skyData;
	f32 timePassed = 0.f;
	f32 speed = 0.06f;
	bool is_paused = false;
};