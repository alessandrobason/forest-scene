// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"

#include <limits>

#include "imGUI/imgui_internal.h"
#include "ImSlider2D.h"

#include "utility.h"
#include "tracelog.h"
#include "vec.h"
#include "MathUtils.h"

static void OptionButton(const char *label, bool &enabled);

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN) {
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	Device *device = renderer->getDevice();
	DeviceContext *ctx = renderer->getDeviceContext();

	// == Initalise scene variables =======================================================================
	
	// -- Texture Manager ---------------------------------------------------------------------------------
	tmanager.init(device, ctx);

	// -- Shader ------------------------------------------------------------------------------------------
	shader         = new DefaultShader(device, hwnd, true);
	treeShader     = new TreeShader(device, hwnd);
	textureShader  = new TextureShader(device, hwnd);
	monolithShader = new MonolithShader(device, hwnd);
	groundShader   = new GroundShader(device, hwnd);

	// -- Render Target -----------------------------------------------------------------------------------
	renderTarget = new RenderTexture(device, screenWidth, screenHeight, 0.1f, 100.f);

	// -- Sky ---------------------------------------------------------------------------------------------
	sky.init(device, ctx, hwnd, &tmanager, &lights[DIR_LIGHT]);

	// -- Bloom -------------------------------------------------------------------------------------------
	bloom.init(device, ctx, screenWidth, screenHeight, hwnd);
	
	// -- Ground ------------------------------------------------------------------------------------------
	ground.init(renderer, hwnd, tmanager);

	// -- Models ------------------------------------------------------------------------------------------
	MModelLoader mloader;
	mloader.init(device, &tmanager);

	treeModel = mloader.load("res/tree.gltf");
	if (!treeModel) err("Couldn't load tree model");

	monolith.moveFromMesh(new CubeMesh(device, ctx));
	mainOrthoMesh.moveFromMesh(new OrthoMesh(device, ctx, screenWidth, screenHeight));

	// -- Lights ------------------------------------------------------------------------------------------
	lights[SPOT_LIGHT].setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	lights[SPOT_LIGHT].setAmbientColour(0.f, 0.f, 0.f, 1.f);
	lights[SPOT_LIGHT].setDirection(0.f, -1.f, 0.f);
	lights[SPOT_LIGHT].setPosition(50.f, 6.f, 50.f);
	lights[SPOT_LIGHT].setSpecularColour(1.f, 1.f, 1.f, 1.f);
	lights[SPOT_LIGHT].setSpecularPower(12.f);
	lights[SPOT_LIGHT].setSpotCutoff(spotCutoff * isTorchOn);

	lights[POINT_LIGHT].setDiffuseColour(monolithColor.x, monolithColor.y, monolithColor.z, 1.0f);
	lights[POINT_LIGHT].setAmbientColour(0.f, 0.f, 0.f, 1.f);
	lights[POINT_LIGHT].setDirection(0.f, 0.f, 1.f);
	lights[POINT_LIGHT].setPosition(0.f, 6.f, 0.f);
	lights[POINT_LIGHT].setSpecularColour(monolithColor.x, monolithColor.y, monolithColor.z, 1.f);
	lights[POINT_LIGHT].setSpecularPower(128.f);
	lights[POINT_LIGHT].setLightStrength(9);

	ground.setWindOrigin(lights[POINT_LIGHT].getPosition());

	// -- Shadows -----------------------------------------------------------------------------------------
	int shadowMapSize = 2048;
	spotShadowMap = new ShadowMap(device, shadowMapSize, shadowMapSize);
	pointShadowMap.init(device, shadowMapSize * 2, shadowMapSize * 2);

	// -- Trees -------------------------------------------------------------------------------------------
	readTreeData();
}

App1::~App1() {
	DELETE_IF_NOT_NULL(shader);
	DELETE_IF_NOT_NULL(treeShader);
	DELETE_IF_NOT_NULL(textureShader);
	DELETE_IF_NOT_NULL(monolithShader);
	DELETE_IF_NOT_NULL(groundShader);
	DELETE_IF_NOT_NULL(renderTarget);
	DELETE_IF_NOT_NULL(treeModel);
	DELETE_IF_NOT_NULL(spotShadowMap);

	DefaultShader::cleanupStaticShaders();

	// Run base application deconstructor
	BaseApplication::~BaseApplication();
}

bool App1::frame() {
	if (!BaseApplication::frame())
		return false;
	
	timePassed += timer->getTime();

	sky.update(timer->getTime());
	ground.update(timer->getTime());
	updateTorchLight();
	
	// Render the graphics.
	if (!render())
		return false;

	return true;
}

bool App1::render() {
	depthPass();
	renderPass();
	bloomPass();
	finalPass();

	return true;
}

void App1::gui() {
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	static bool showTreesOpts    = false;
	static bool showMonolithOpts = false;
	static bool showSkyOpts      = false;
	static bool showGroundOpts   = false;
	static bool showLightsOpts   = false;
	static bool isFirst          = false;

	// when debugging for some reason static bools where true at startup no matter what
	// in case it happens again, turn them all to false in the first frame
	if (isFirst) {
		showTreesOpts    = false;
		showMonolithOpts = false;
		showSkyOpts      = false;
		showGroundOpts   = false;
		showLightsOpts   = false;
		isFirst          = false;
	}

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	OptionButton("Show trees options", showTreesOpts);
	OptionButton("Show monolith options", showMonolithOpts);
	OptionButton("Show sky options", showSkyOpts);
	OptionButton("Show ground options", showGroundOpts);
	OptionButton("Show lights options", showLightsOpts);
	
	if (showTreesOpts) {
		ImGui::Begin("Trees options", &showTreesOpts);
		ImGui::SliderFloat("Wave amplitude", &treeAmplitude, 0.001f, 0.07f);
		ImGui::SliderFloat("Wind speed", &treeSpeed, 0.0f, 10.f);
		if (ImGui::Button("Reload tree data file")) {
			readTreeData();
		}
		ImGui::End();
	}

	if (showMonolithOpts) {
		ImGui::Begin("Monolith options", &showMonolithOpts);

		ImGui::SliderFloat("Width", &monolithScale.x, 0.1f, 15.f);
		ImGui::SliderFloat("Height", &monolithScale.y, 1.f, 15.f);
		ImGui::SliderFloat("Thickness", &monolithScale.z, 1.f, 15.f);
		ImGui::SliderFloat("Angle", &monolithAngle, 0.f, 360.f, "%3.fdeg");
		bloom.gui();

		ImGui::End();
	}
	
	if (showSkyOpts) {
		sky.gui(showSkyOpts);
	}

	if (showGroundOpts) {
		ground.gui(showGroundOpts);
	}

	if (showLightsOpts) {
		ImGui::Begin("Lights options", &showLightsOpts);

		// -- Directional light ----------------------------------------------------------------
		static vec4f dirAmb    = lights[DIR_LIGHT].getAmbientColour();
		static vec4f dirDif    = lights[DIR_LIGHT].getDiffuseColour();
		static vec4f dirSpeCol = lights[DIR_LIGHT].getSpecularColour();
		static bool dirDifSpe  = false;
		static float dirSpePow = lights[DIR_LIGHT].getSpecularPower();
		bool changed = false;
		
		ImGui::PushID("Directional");
		ImGui::Text("Directional light");
		ImGui::Separator();
		changed |= ImGui::ColorEdit3("Ambient", (f32 *)&dirAmb);
		changed |= ImGui::ColorEdit3("Diffuse", (f32 *)&dirDif);
		changed |= ImGui::ColorEdit3("Specular", (f32 *)&dirSpeCol);
		changed |= ImGui::Checkbox("Diffuse and specular color are the same", &dirDifSpe);
		changed |= ImGui::SliderFloat("Specular power", &dirSpePow, 0.01f, 200.f);

		if (changed) {
			if (dirDifSpe) {
				dirSpeCol = dirDif;
			}

			lights[DIR_LIGHT].setAmbientColour(dirAmb.x, dirAmb.y, dirAmb.z, dirAmb.w);
			lights[DIR_LIGHT].setDiffuseColour(dirDif.x, dirDif.y, dirDif.z, dirDif.w);
			lights[DIR_LIGHT].setSpecularColour(dirSpeCol.x, dirSpeCol.y, dirSpeCol.z, dirSpeCol.w);
			lights[DIR_LIGHT].setSpecularPower(dirSpePow);
		}

		ImGui::PopID();

		// -- Spot light -----------------------------------------------------------------------
		static vec4f spoAmb    = lights[SPOT_LIGHT].getAmbientColour();
		static vec4f spoDif    = lights[SPOT_LIGHT].getDiffuseColour();
		static vec4f spoSpeCol = lights[SPOT_LIGHT].getSpecularColour();
		static bool spoDifSpe  = true;
		static int spoStrength = lights[SPOT_LIGHT].getLightStrength();
		static float spoSpePow = lights[SPOT_LIGHT].getSpecularPower();
		changed = false;

		ImGui::PushID("Spot");
		ImGui::NewLine();
		ImGui::Text("Spot light");
		ImGui::Separator();
		changed |= ImGui::ColorEdit3("Ambient", (f32 *)&spoAmb);
		changed |= ImGui::ColorEdit3("Diffuse", (f32 *)&spoDif);
		changed |= ImGui::ColorEdit3("Specular", (f32 *)&spoSpeCol);
		changed |= ImGui::Checkbox("Diffuse and specular color are the same", &spoDifSpe);
		changed |= ImGui::SliderFloat("Specular power", &spoSpePow, 0.01f, 200.f);
		changed |= ImGui::SliderInt("Light strength", &spoStrength, 1, 11);
		changed |= ImGui::SliderFloat("Spot cutoff", &spotCutoff, 0.f, 50.f, "%.3fdeg");

		if (changed) {
			if (spoDifSpe) {
				spoSpeCol = spoDif;
			}

			lights[SPOT_LIGHT].setAmbientColour(spoAmb.x, spoAmb.y, spoAmb.z, spoAmb.w);
			lights[SPOT_LIGHT].setDiffuseColour(spoDif.x, spoDif.y, spoDif.z, spoDif.w);
			lights[SPOT_LIGHT].setSpecularColour(spoSpeCol.x, spoSpeCol.y, spoSpeCol.z, spoSpeCol.w);
			lights[SPOT_LIGHT].setSpecularPower(spoSpePow);
			lights[SPOT_LIGHT].setLightStrength(spoStrength);
			lights[SPOT_LIGHT].setSpotCutoff(spotCutoff);
		}

		ImGui::PopID();

		// -- Point light ----------------------------------------------------------------------

		static vec4f poiAmb    = lights[POINT_LIGHT].getAmbientColour();
		static vec4f poiSpeCol = lights[POINT_LIGHT].getSpecularColour();
		static float poiSpePow = lights[POINT_LIGHT].getSpecularPower();
		static bool poiDifSpe  = true;
		static int poiStrength = lights[POINT_LIGHT].getLightStrength();
		static vec3f pos       = lights[POINT_LIGHT].getPosition();
		static ImVec2 imPos    = { pos.x, pos.z }; 
		changed = false;

		ImGui::PushID("Point");
		ImGui::NewLine();
		ImGui::Text("Point light");
		ImGui::Separator();
		changed |= ImGui::ColorEdit3("Ambient", (f32 *)&poiAmb);
		changed |= ImGui::ColorEdit3("Diffuse", (f32 *)&monolithColor);
		changed |= ImGui::ColorEdit3("Specular", (f32 *)&poiSpeCol);
		changed |= ImGui::Checkbox("Diffuse and specular color are the same", &poiDifSpe);
		changed |= ImGui::SliderFloat("Specular power", &poiSpePow, 0.01f, 200.f);
		changed |= ImGui::SliderInt("Light strength", &poiStrength, 1, 11);
		changed |= ImGui::InputVec2("Position", &imPos, { -100.f, -100.f }, { 100.f, 100.f });

		if (changed) {
			if (poiDifSpe) {
				poiSpeCol = monolithColor;
			}

			pos.x = imPos.x;
			pos.z = imPos.y;

			lights[POINT_LIGHT].setAmbientColour(poiAmb.x, poiAmb.y, poiAmb.z, poiAmb.w);
			lights[POINT_LIGHT].setDiffuseColour(monolithColor.x, monolithColor.y, monolithColor.z, 1.f);
			lights[POINT_LIGHT].setSpecularColour(poiSpeCol.x, poiSpeCol.y, poiSpeCol.z, poiSpeCol.w);
			lights[POINT_LIGHT].setSpecularPower(poiSpePow);
			lights[POINT_LIGHT].setLightStrength(poiStrength);
			lights[POINT_LIGHT].setPosition(pos.x, pos.y, pos.z);

			ground.setWindOrigin(pos);
		}

		ImGui::PopID();
		
		ImGui::End();
	}

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void App1::updateTorchLight() {
	bool isPressed = input->isKeyDown('F');
	if (isPressed && !wasPressed) {
		lights[SPOT_LIGHT].setSpotCutoff(spotCutoff * isTorchOn);
		isTorchOn = !isTorchOn;
	}
	wasPressed = isPressed;

	vec3f rot = degToRad(vec3f(camera->getRotation()));
	mat4 rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	vec3f dir = mul(rotMat, float3(0, 0, 1));
	dir.normalize();
	vec3f pos = camera->getPosition();
	pos += dir * 2;
	pos.y -= 1.5f;
	lights[SPOT_LIGHT].setPosition(pos.x, pos.y, pos.z);
	lights[SPOT_LIGHT].setDirection(dir.x, dir.y, dir.z);
}

void App1::renderScene(const mat4 &world, const mat4 &view, const mat4 &proj) {
	float3 cameraPos = camera->getPosition();
	
	// -- Render trees ----------------------------------------------------------------------
	for (MMesh &mesh : treeModel->meshes) {
		treeShader->setShaderParameters(
			renderer->getDeviceContext(),
			world, view, proj,
			tmanager.getTexture(mesh.textureId),
			mesh.diffuseColor,
			cameraPos,
			timePassed,
			lights,
			spotShadowMap, pointShadowMap,
			lights[POINT_LIGHT].getPosition(),
			treeAmplitude, treeSpeed
		);
		treeShader->renderInstance(
			renderer->getDevice(),
			renderer->getDeviceContext(),
			mesh,
			treeData.data(),
			(uint)treeData.size()
		);
	}

	// -- Render monolith -------------------------------------------------------------------
	if (!monolithShader->isOmniShader()) {
		float3 pos = lights[POINT_LIGHT].getPosition();
		pos.y = monolithScale.y;
		lights[POINT_LIGHT].setPosition(pos.x, pos.y, pos.z);

		mat4 monolightTran = XMMatrixTranslation(pos.x, pos.y, pos.z);
		mat4 monolightScale = XMMatrixScaling(monolithScale.x, monolithScale.y, monolithScale.z);
		mat4 monoRot = XMMatrixRotationY(degToRad(monolithAngle));

		monolithShader->setShaderParameters(
			renderer->getDeviceContext(),
			monolightScale * (monoRot * monolightTran * world), view, proj,
			monolithColor
		);
		monolithShader->render(renderer->getDeviceContext(), monolith);
	}
	
	// -- Render plane ----------------------------------------------------------------------
	ground.renderGround(
		renderer->getDeviceContext(),
		view, proj, cameraPos,
		lights, 
		spotShadowMap, pointShadowMap
	);
}

void App1::depthPass() {
	// Turn on depth shaders
	treeShader->useDepthShader(true);
	monolithShader->useDepthShader(true);
	groundShader->useDepthShader(true);
	ground.getGrassShader()->useDepthShader(true);
	ground.getGroundShader()->useDepthShader(true);
	shader->useDepthShader(true);

	mat4 world = renderer->getWorldMatrix();
	mat4 view, proj;

	//-- Spot light --------------------------------------------------------------
	lights[SPOT_LIGHT].generateViewMatrix();
	lights[SPOT_LIGHT].generateProjectionMatrix(0.1f, 100.f);
	view = lights[SPOT_LIGHT].getViewMatrix();
	proj = lights[SPOT_LIGHT].getProjectionMatrix();

	// bind shadow map's render target
	spotShadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
	renderScene(world, view, proj);

	//-- Point light -------------------------------------------------------------
	constexpr f32 aspect = 1.f;
	constexpr f32 zNear  = 1.f;
	constexpr f32 zFar   = 200.f;
	constexpr vec3f lightDir[6] = {
		{  1.f,  0.f,  0.f }, { -1.f,  0.f,  0.f }, {  0.f,  1.f,  0.f },
		{  0.f, -1.f,  0.f }, {  0.f,  0.f,  1.f }, {  0.f,  0.f, -1.f },
	};
	constexpr vec3f lightUp[6] = {
		{ 0.f,  1.f,  0.f }, { 0.f,  1.f,  0.f }, { 0.f,  0.f,  1.f },
		{ 0.f,  0.f, -1.f }, { 0.f,  1.f,  0.f }, { 0.f,  1.f,  0.f },
	};

	treeShader->useOmniDepthShader(true);
	monolithShader->useOmniDepthShader(true);
	groundShader->useOmniDepthShader(true);
	ground.getGrassShader()->useOmniDepthShader(true);
	ground.getGroundShader()->useOmniDepthShader(true);
	shader->useOmniDepthShader(true);

	vec3f lightPos = lights[POINT_LIGHT].getPosition();
	for (int i = 0; i < 6; ++i) {
		view = lookAt(lightPos, lightPos + lightDir[i], lightUp[i]);
		pointShadowMap.setViewMatrix(view, i);
	}

	proj = XMMatrixPerspectiveFovLH(degToRad(90.f), aspect, zNear, zFar);

	pointShadowMap.bind(renderer->getDeviceContext());
	renderScene(world, XMMatrixIdentity(), proj);

	treeShader->useOmniDepthShader(false);
	monolithShader->useOmniDepthShader(false);
	groundShader->useOmniDepthShader(false);
	ground.getGrassShader()->useOmniDepthShader(false);
	ground.getGroundShader()->useOmniDepthShader(false);
	shader->useOmniDepthShader(false);

	//----------------------------------------------------------------------------
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();

	// turn off depth shaders
	treeShader->useDepthShader(false);
	monolithShader->useDepthShader(false);
	groundShader->useDepthShader(false);
	ground.getGrassShader()->useDepthShader(false);
	ground.getGroundShader()->useDepthShader(false);
	shader->useDepthShader(false);
}

void App1::renderPass() {
	renderer->setWireframeMode(wireframeToggle);
	renderTarget->setRenderTarget(renderer->getDeviceContext());
	renderTarget->clearRenderTarget(renderer->getDeviceContext(), 0.39f, 0.58f, 0.92f, 1.0f);

	camera->update();
	mat4 world = renderer->getWorldMatrix();
	mat4 view  = camera->getViewMatrix();
	mat4 proj  = renderer->getProjectionMatrix();

	sky.render(renderer, camera);
	renderScene(world, view, proj);
	ground.renderGrass(
		renderer->getDeviceContext(),
		view, proj,
		camera->getPosition(),
		lights,
		spotShadowMap, pointShadowMap
	);

	renderer->setBackBufferRenderTarget();
	renderer->setWireframeMode(false);
}

void App1::bloomPass() {
	mat4 world = renderer->getWorldMatrix();
	mat4 proj = renderer->getOrthoMatrix();
	mat4 view = camera->getOrthoViewMatrix();

	renderer->setZBuffer(false);
	bloomResult = bloom.apply(
		renderer->getDeviceContext(),
		world, view, proj,
		renderTarget->getShaderResourceView()
	);
	renderer->setZBuffer(true);

	renderer->setBackBufferRenderTarget();
}

void App1::finalPass() {
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);

	// get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	mat4 world = renderer->getWorldMatrix();
	mat4 proj  = renderer->getOrthoMatrix();
	mat4 view  = camera->getOrthoViewMatrix();

	renderer->setZBuffer(false);
	
	// Render scene from renderTarget to a full-screen orthographic mesh
	textureShader->setShaderParameters(
		renderer->getDeviceContext(),
		world, view, proj,
		bloomResult
	);
	textureShader->render(renderer->getDeviceContext(), mainOrthoMesh);

	renderer->setZBuffer(true);

	gui();
	renderer->endScene();
}

void App1::readTreeData() {
	// Tries to load a tree_data file, the format is simply values divided by a white space
	// if it can load the file for whatever reason it loads some fallback positions

	treeData.clear();

	std::ifstream file("res/tree_data.txt");
	if (!file.good()) {
		err("couldn't open tree data file");
		treeDataFallback();
		return;
	}
	
	f32 x, y;
	while (file >> x >> y) {
		treeData.emplace_back(x, 0.1f, y);
	}
}

void App1::treeDataFallback() {
	treeData.emplace_back(-12.730f, 0.f,  47.556f);
	treeData.emplace_back(-65.968f, 0.f, -24.806f);
	treeData.emplace_back(-48.676f, 0.f,   8.661f);
	treeData.emplace_back(-38.277f, 0.f,  -5.602f);
	treeData.emplace_back( -4.338f, 0.f,  26.650f);
	treeData.emplace_back( 20.907f, 0.f,   3.277f);
	treeData.emplace_back( 12.519f, 0.f, -19.656f);
	treeData.emplace_back(-22.143f, 0.f, -35.033f);
	treeData.emplace_back( 20.597f, 0.f,  14.886f);
	treeData.emplace_back( 38.216f, 0.f, -27.661f);
	treeData.emplace_back(-15.243f, 0.f, -16.711f);
	treeData.emplace_back( -6.433f, 0.f,  12.854f);
	treeData.emplace_back( 44.561f, 0.f, -10.193f);
	treeData.emplace_back(-46.925f, 0.f,  44.733f);
	treeData.emplace_back( 45.502f, 0.f,  13.071f);
	treeData.emplace_back(-55.887f, 0.f,  25.213f);
	treeData.emplace_back(-71.223f, 0.f,  13.579f);
	treeData.emplace_back( 30.166f, 0.f,   1.437f);
	treeData.emplace_back(-62.261f, 0.f,  33.099f);
	treeData.emplace_back( 37.026f, 0.f, -18.019f);
	treeData.emplace_back(-31.492f, 0.f,   0.569f);
	treeData.emplace_back(-30.579f, 0.f, -28.345f);
	treeData.emplace_back( 22.880f, 0.f, -39.295f);
	treeData.emplace_back(  6.996f, 0.f,  23.534f);
	treeData.emplace_back(-37.478f, 0.f, -46.667f);
	treeData.emplace_back( -2.816f, 0.f, -31.290f);
	treeData.emplace_back( 14.724f, 0.f,  -8.805f);
	treeData.emplace_back(-19.673f, 0.f,  15.016f);
	treeData.emplace_back(-51.256f, 0.f, -16.995f);
	treeData.emplace_back(-64.011f, 0.f, - 2.973f);
}

static void OptionButton(const char *label, bool &enabled) {
	bool was = enabled;

	if (was) {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	if (ImGui::Button(label)) {
		enabled = true;
	}

	if (was) {
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}
}
