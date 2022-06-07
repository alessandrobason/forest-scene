#include "GrassShader.h"

#include "utility.h"
#include "tracelog.h"
#include "vec.h"
#include "MathUtils.h"

GrassShader::GrassShader(Device *device, HWND hwnd) 
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/grass_vs.cso", L"shaders/ground_hs.cso", L"shaders/grass_ds.cso", L"shaders/grass_gs.cso", L"shaders/grass_ps.cso");
}

GrassShader::~GrassShader() {
	RELEASE_IF_NOT_NULL(grassBuffer);
	RELEASE_IF_NOT_NULL(groundBuffer);
}

void GrassShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &projection, 
	TextureType *grassPattern, 
	TextureType *grassTexture, 
	const WindData &windData,
	const float3 &camPos, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow,
	OmniShadowMap &pointShadow,
	f32 cutoffDist,
	f32 maxDist, 
	f32 maxFactor
) {
	// == GROUND BUFFER ==========================

	auto groundPtr = mapBuffer<GroundBufferType>(ctx, groundBuffer);
	groundPtr->cameraPos  = camPos;
	groundPtr->cutoffDist = cutoffDist;
	groundPtr->maxDist    = maxDist;
	groundPtr->maxFactor  = maxFactor;
	unmapBufferHS(ctx, groundBuffer, 0);

	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(projection);

	// -- Geometry shader -----------------------
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferGS(ctx, matrixBuffer, 0);

	// -- Domain shader -------------------------
	matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferDS(ctx, matrixBuffer, 0);

	// == DEFAULT BUFFER ========================

	auto camPtr = mapBuffer<DefaultBufferType>(ctx, defaultBuffer);
	camPtr->cameraPos = camPos;
	camPtr->timePassed = windData.timePassed;
	unmapBufferGS(ctx, defaultBuffer, 1);

	// == GRASS BUFFER ==========================

	auto grassPtr = mapBuffer<GrassBufferType>(ctx, grassBuffer);
	grassPtr->timePassed = windData.timePassed;
	grassPtr->windOrigin = windData.windOrigin;
	grassPtr->windSpeed = windData.windSpeed;
	grassPtr->waveAmplitude = windData.waveAmplitude;
	grassPtr->padding = { 0, 0 };
	mat4 spotView = lights[SPOT_LIGHT].getViewMatrix();
	mat4 spotProj = lights[SPOT_LIGHT].getProjectionMatrix();
	grassPtr->spotLightMVP = XMMatrixTranspose(spotView * spotProj);
	unmapBufferGS(ctx, grassBuffer, 2);

	if (!isDepth) {
		// == LIGHT BUFFER ===========================
		auto lightPtr = mapBuffer<LightBufferType>(ctx, lightBuffer);
		lightPtr->pointLightPos = lights[POINT_LIGHT].getPosition();
		for (int i = 0; i < LIGHTS_COUNT; ++i) {
			switch (i) {
			case DIR_LIGHT:
				lightPtr->dirSpotData[i].lightDir = lights[i].getDirection();
				lightPtr->dirSpotData[i].spotCutoff = 0.f;
				break;
			case SPOT_LIGHT:
				lightPtr->dirSpotData[i].lightDir = lights[i].getDirection();
				lightPtr->dirSpotData[i].spotCutoff = lights[i].getSpotCutoff();
				break;
			}

			auto &lightData = lightPtr->sharedLightData[i];
			auto &factors = lightPtr->factor[i];
			auto &factor = lightFactors[lights[i].getLightStrength()];

			lightData.ambient = lights[i].getAmbientColour();
			lightData.diffuse = lights[i].getDiffuseColour();
			lightData.specular = lights[i].getSpecularColour();
			float3 pos = lights[i].getPosition();
			lightData.position = { pos.x, pos.y, pos.z, 1.f };

			factors.specularPower = lights[i].getSpecularPower();
			factors.constant = factor.constant;
			factors.linear = factor.linear;
			factors.quadratic = factor.quadratic;
		}
		unmapBufferPS(ctx, lightBuffer, 0);

		// == SHADER RESOURCES =======================

		TextureType *spotTexture  = spotShadow->getDepthMapSRV();
		TextureType *pointTexture = pointShadow.getCubemap();

		// Set shader texture resource in the pixel shader.
		ctx->PSSetShaderResources(0, 1, &grassTexture);
		ctx->PSSetShaderResources(1, 1, &spotTexture);
		ctx->PSSetShaderResources(2, 1, &pointTexture);
		ctx->PSSetSamplers(0, 1, &sampleState);
		ctx->PSSetSamplers(1, 1, &shadowMapSampler);
	}

	// == GEOMETRY SHADER RESOURCES =============
	ctx->GSSetShaderResources(0, 1, &grassPattern);
	ctx->GSSetSamplers(0, 1, &sampleState);
}

void GrassShader::render(DeviceContext *ctx, MMesh &mesh) {
	// == SEND DATA ==============================

	ID3D11Buffer *vbuf = mesh.getVertexBuffer();
	ID3D11Buffer *ibuf = mesh.getIndexBuffer();
	uint stride = sizeof(MMesh::PubVertexType);
	uint offset = 0;

	ctx->IASetVertexBuffers(0, 1, &vbuf, &stride, &offset);
	ctx->IASetIndexBuffer(ibuf, DXGI_FORMAT_R32_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	// == RENDER =================================

	// Set the vertex input layout.
	ctx->IASetInputLayout(layout);

	// Set the vertex and pixel shaders that will be used to render.
	if (isDepth) {
		ctx->VSSetShader(vertexDepthShader, NULL, 0);
		ctx->PSSetShader(NULL, NULL, 0);
	}
	else {
		ctx->VSSetShader(vertexShader, NULL, 0);
		ctx->PSSetShader(pixelShader, NULL, 0);
	}
	ctx->CSSetShader(NULL, NULL, 0);

	// if Hull shader is not null then set HS and DS
	if (hullShader) {
		ctx->HSSetShader(hullShader, NULL, 0);
		ctx->DSSetShader(domainShader, NULL, 0);
	}
	else {
		ctx->HSSetShader(NULL, NULL, 0);
		ctx->DSSetShader(NULL, NULL, 0);
	}

	// if geometry shader is not null then set GS
	if (geometryShader) {
		ctx->GSSetShader(geometryShader, NULL, 0);
	}
	else {
		ctx->GSSetShader(NULL, NULL, 0);
	}

	if (isOmni) {
		ctx->GSSetShader(omniDepthGSShader, NULL, 0);
	}

	// Render the triangle.
	ctx->DrawIndexed(mesh.getIndexCount(), 0, 0);

	// unbind shader resources
	TextureType *nullTEX[LIGHTS_COUNT + 1] = { 0 };
	ctx->PSSetShaderResources(0, LIGHTS_COUNT + 1, nullTEX);
}

void GrassShader::initShader(
	const wchar_t *vs, 
	const wchar_t *hs, 
	const wchar_t *ds, 
	const wchar_t *gs, 
	const wchar_t *ps
) {
	loadVertexShader(vs);
	loadDomainShader(ds);
	loadHullShader(hs);
	loadGeometryShader(gs);
	loadPixelShader(ps);

	loadDepthShader(vs);

	addDynamicBuffer<GrassBufferType>(&grassBuffer);
	addDynamicBuffer<GroundBufferType>(&groundBuffer);
	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<DefaultBufferType>(&defaultBuffer);
	addDynamicBuffer<LightBufferType>(&lightBuffer);

	addDiffuseSampler();
	addShadowSampler();
}

/*
void Grass::init(D3D *ctx, HWND hwnd, TextureIdManager &textureManager) {
	windData.timePassed = 0.f;
	windData.windOrigin = { 0.f, 0.f, 0.f };
	windData.windSpeed = 3.f;
	windData.waveAmplitude = 10.f;

	grassShader = new GrassShader(ctx->getDevice(), hwnd);
	//plane.moveFromMesh(new PlaneMesh(ctx->getDevice(), ctx->getDeviceContext(), 200));

	tmanager = &textureManager;
	grassTextureId = tmanager->loadTexture("res/grass.png");
	grassPatternId = tmanager->loadTexture("res/grassPattern.png");

	//planeMat = XMMatrixTranslation(-100.f, 0.f, -100.f);
}

Grass::~Grass() {
	DELETE_IF_NOT_NULL(grassShader);
	tmanager = nullptr;
}

void Grass::update(f32 dt) {
	windData.timePassed += dt;
}

void Grass::render(
	DeviceContext *ctx, 
	const mat4 &planeMatrix,
	const mat4 &view, 
	const mat4 &proj,
	const float3 &camPos, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow,
	OmniShadowMap &pointShadow
) {
	grassShader->setShaderParameters(
		ctx, planeMatrix, view, proj,
		tmanager->getTexture(grassPatternId),
		tmanager->getTexture(grassTextureId),
		windData,
		camPos, lights, 
		spotShadow, pointShadow
	);
	grassShader->render(ctx, plane);
}

void Grass::gui(bool &open) {
	ImGui::Begin("Grass options", &open);

	ImGui::SliderFloat("Wind speed", &windData.windSpeed, 1.f, 30.f);
	ImGui::SliderFloat("Wind wave amplitude", &windData.waveAmplitude, 1.f, 30.f);

	if (ImGui::Button("Reload base texture")) {
		if (!tmanager->reloadTexture("res/grassPattern.png", grassPatternId)) {
			err("Couldn't reload grass pattern texture!");
			grassPatternId = -1;
		}
	}

	ImGui::End();
}

void Grass::setWindOrigin(const float3 &origin) {
	windData.windOrigin = mul(XMMatrixInverse(nullptr, planeMat), origin);
}
*/