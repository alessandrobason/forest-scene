// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include <array>

#include "DXF.h"
#include "DefaultShader.h"
#include "TreeShader.h"
#include "InstanceShader.h"
#include "TextureShader.h"
#include "MonolithShader.h"
#include "Sky.h"
#include "Bloom.h"
#include "Ground.h"
#include "TextureIdManager.h"
#include "mmodel.h"
#include "vec.h"
#include "OmniShadowMap.h"

class App1 : public BaseApplication {
public:
	App1() = default;
	~App1() override;
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void gui();

	void updateTorchLight();

	void renderScene(const mat4 &world, const mat4 &view, const mat4 &proj);
	void depthPass();
	void renderPass();
	void bloomPass();
	void finalPass();

	void readTreeData();
	void treeDataFallback();

private:
	DefaultShader *shader = nullptr;
	TreeShader *treeShader = nullptr;
	TextureShader *textureShader = nullptr;
	MonolithShader *monolithShader = nullptr;
	GroundShader *groundShader = nullptr;
	TextureIdManager tmanager;
	
	RenderTexture *renderTarget = nullptr;
	TextureType *bloomResult = nullptr;
	MMesh mainOrthoMesh;

	Sky sky;
	Bloom bloom;
	Ground ground;

	Light lights[LIGHTS_COUNT];
	ShadowMap *spotShadowMap = nullptr;
	OmniShadowMap pointShadowMap;
	MModel *treeModel = nullptr;

	MMesh monolith;
	vec3f monolithScale { 4, 7, 1 };
	vec4f monolithColor { 0.2f, 0.7f, 1.0f, 1.0f };
	f32 monolithAngle = 0.f;

	float timePassed = 0.f;

	f32 spotCutoff = 20.f;
	bool isTorchOn = false;
	bool wasPressed = false;

	f32 treeAmplitude = 0.04f; 
	f32 treeSpeed = 1.f;

	std::vector<TreeInstanceType> treeData;
};

#endif