#include "Ground.h"

#include "utility.h"
#include "tracelog.h"
#include "MathUtils.h"

GroundShader::GroundShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/ground_vs.cso", L"shaders/ground_hs.cso", L"shaders/ground_ds.cso", L"shaders/ground_vs_depth.cso");
}

GroundShader::~GroundShader() {
	RELEASE_IF_NOT_NULL(groundBuffer);
}

void GroundShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &proj,
	TextureType *texture, 
	float4 color, 
	float3 cameraPos, 
	f32 timePassed, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow, 
	OmniShadowMap &pointShadow, 
	f32 cutoffDist,
	f32 maxDist,
	f32 maxFactor
) {
	// == GROUND BUFFER ==========================

	auto groundPtr = mapBuffer<GroundBufferType>(ctx, groundBuffer);
	groundPtr->cameraPos  = cameraPos;
	groundPtr->cutoffDist = cutoffDist;
	groundPtr->maxDist    = maxDist;
	groundPtr->maxFactor  = maxFactor;
	unmapBufferHS(ctx, groundBuffer, 0);

	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(proj);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferDS(ctx, matrixBuffer, 0);

	// == DEFAULT BUFFER ========================

	auto camPtr = mapBuffer<DefaultBufferType>(ctx, defaultBuffer);
	camPtr->cameraPos = cameraPos;
	camPtr->timePassed = timePassed;
	unmapBufferDS(ctx, defaultBuffer, 1);

	// == SHADOW BUFFER =========================
	
	auto shadowPtr = mapBuffer<ShadowBufferType>(ctx, shadowBuffer);
	mat4 spotLightView = lights[SPOT_LIGHT].getViewMatrix();
	mat4 spotLightProj = lights[SPOT_LIGHT].getProjectionMatrix();
	shadowPtr->spotLightMVP = XMMatrixTranspose(spotLightView * spotLightProj);
	unmapBufferDS(ctx, shadowBuffer, 2);


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


		// == MATERIAL BUFFER ========================

		auto matPtr = mapBuffer<MaterialBufferType>(ctx, materialBuffer);
		matPtr->diffuseColor = color;
		unmapBufferPS(ctx, materialBuffer, 1);

		// == SHADER RESOURCES =======================

		TextureType *spotTexture  = spotShadow->getDepthMapSRV();
		TextureType *pointTexture = pointShadow.getCubemap();

		// Set shader texture resource in the pixel shader.
		ctx->PSSetShaderResources(0, 1, &texture);
		ctx->PSSetShaderResources(1, 1, &spotTexture);
		ctx->PSSetShaderResources(2, 1, &pointTexture);
		ctx->PSSetSamplers(0, 1, &sampleState);
		ctx->PSSetSamplers(1, 1, &shadowMapSampler);
	}
}

void GroundShader::initShader(
	const wchar_t *vs, 
	const wchar_t *hs, 
	const wchar_t *ds, 
	const wchar_t *dvs
) {
	loadVertexShader(vs);
	loadDepthShader(dvs);
	loadHullShader(hs);
	loadDomainShader(ds);
	pixelShader = getDefaultPixelShader(this);

	addDynamicBuffer<GroundBufferType>(&groundBuffer);
	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<DefaultBufferType>(&defaultBuffer);
	addDynamicBuffer<ShadowBufferType>(&shadowBuffer);
	addDynamicBuffer<LightBufferType>(&lightBuffer);
	addDynamicBuffer<MaterialBufferType>(&materialBuffer);
	addDiffuseSampler();
	addShadowSampler();
}

void GroundShader::render(DeviceContext *ctx, MMesh &mesh) {
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

	// Render the triangle.
	ctx->DrawIndexed(mesh.getIndexCount(), 0, 0);

	// unbind shader resources
	TextureType *nullTEX[LIGHTS_COUNT + 1] = { 0 };
	ctx->PSSetShaderResources(0, LIGHTS_COUNT + 1, nullTEX);
}

void GroundMesh::init(Device *device, DeviceContext *ctx, vec2i planeSize, vec2i planeRes) {
	vec4f tex = vec4f::zero();
	tex.setSize({ 1.f / planeRes.x, 1.f / planeRes.y });
	vec2f texIncr = { 1.f / planeRes.x, 1.f / planeRes.y };

	vec2f size = (vec2f)planeSize / planeRes;
	vec2f startPos = -planeSize / 2;

	vec4f cur = vec4f::zero();
	cur.setPosition(startPos);
	cur.setSize(size);

	int index = 0;

	// Calculate the number of vertices in the terrain mesh.
	vertexCount = planeRes.x * planeRes.y * 6;
	indexCount = vertexCount;

	VertexType *vertices = new VertexType[vertexCount];
	unsigned long *indices = new unsigned long[indexCount];

#define AS_FLOAT3(vec) float3(vec.x, 0.f, vec.y)

	for (int j = 0; j < planeRes.y; ++j) {
		for (int i = 0; i < planeRes.x; ++i) {
			// Top left.
			vertices[index + 0].position = AS_FLOAT3(cur.topLeft());
			vertices[index + 0].texture = tex.topLeft();
			vertices[index + 0].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Bottom right.
			vertices[index + 1].position = AS_FLOAT3(cur.bottomRight());
			vertices[index + 1].texture = tex.bottomRight();
			vertices[index + 1].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Bottom left
			vertices[index + 2].position = AS_FLOAT3(cur.bottomLeft());
			vertices[index + 2].texture = tex.bottomLeft();
			vertices[index + 2].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Top left
			vertices[index + 3].position = AS_FLOAT3(cur.topLeft());
			vertices[index + 3].texture = tex.topLeft();
			vertices[index + 3].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Bottom right
			vertices[index + 4].position = AS_FLOAT3(cur.topRight());
			vertices[index + 4].texture = tex.topRight();
			vertices[index + 4].normal = XMFLOAT3(0.0, 1.0, 0.0);

			// Top right.
			vertices[index + 5].position = AS_FLOAT3(cur.bottomRight());
			vertices[index + 5].texture = tex.bottomRight();
			vertices[index + 5].normal = XMFLOAT3(0.0, 1.0, 0.0);

			indices[index + 0] = index + 0;
			indices[index + 1] = index + 1;
			indices[index + 2] = index + 2;
			indices[index + 3] = index + 3;
			indices[index + 4] = index + 4;
			indices[index + 5] = index + 5;
			index += 6;

			tex.x += texIncr.x;
			cur.x += size.x;
		}

		tex.x = 0.f;
		tex.y += texIncr.y;

		cur.x = startPos.x;
		cur.y += size.y;
	}

#undef AS_FLOAT3

	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData{};
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	// Now create the vertex buffer.
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc{};
	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData{};
	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	// Create the index buffer.
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Release the arrays now that the buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}

void Ground::init(D3D *ctx, HWND hwnd, TextureIdManager &textureManager) {
	windData.timePassed = 0.f;
	windData.windOrigin = { 0.f, 0.f, 0.f };
	windData.windSpeed = 3.f;
	windData.waveAmplitude = 10.f;

	grassShader = new GrassShader(ctx->getDevice(), hwnd);
	groundShader = new GroundShader(ctx->getDevice(), hwnd);

	tmanager = &textureManager;
	grassTextureId = tmanager->loadTexture("res/grass.png");
	grassPatternId = tmanager->loadTexture("res/grassPattern.png");
	groundTextureId = tmanager->loadTexture("res/ground.jpg");

	ground.init(ctx->getDevice(), ctx->getDeviceContext());
}

Ground::~Ground() {
	DELETE_IF_NOT_NULL(grassShader);
	DELETE_IF_NOT_NULL(groundShader);
	tmanager = nullptr;
}

void Ground::update(f32 dt) {
	windData.timePassed += dt;
}

void Ground::renderGrass(
	DeviceContext *ctx, 
	const mat4 &view, 
	const mat4 &proj, 
	const float3 &camPos, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow, 
	OmniShadowMap &pointShadow
) {
	if (shouldDrawGrass) {
		grassShader->setShaderParameters(
			ctx, groundMatrix, view, proj,
			tmanager->getTexture(grassPatternId),
			tmanager->getTexture(grassTextureId),
			windData,
			camPos, lights,
			spotShadow, pointShadow,
			cutoffDistance, maxDistance, maxFactor
		);
		grassShader->render(ctx, ground);
	}
}

void Ground::renderGround(
	DeviceContext *ctx, 
	const mat4 &view, 
	const mat4 &proj, 
	const float3 &camPos, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow, 
	OmniShadowMap &pointShadow
) {
	if (groundShader->isOmniShader()) return;

	groundShader->setShaderParameters(
		ctx, groundMatrix, view, proj,
		tmanager->getTexture(groundTextureId),
		{},
		camPos, windData.timePassed,
		lights,
		spotShadow, pointShadow,
		cutoffDistance, maxDistance, maxFactor
	);
	groundShader->render(ctx, ground);
}

void Ground::gui(bool &open) {
	ImGui::Begin("Ground options", &open);

	// -- Grass options -------------------------------------------------------
	ImGui::Text("Grass options");
	ImGui::Separator();

	ImGui::Checkbox("Render", &shouldDrawGrass);
	ImGui::SliderFloat("Wind speed", &windData.windSpeed, 1.f, 30.f);
	ImGui::SliderFloat("Wind wave amplitude", &windData.waveAmplitude, 1.f, 30.f);

	ImGui::Image(tmanager->getTexture(grassPatternId), { 100.f, 100.f });
	if (ImGui::Button("Reload grass pattern texture")) {
		if (!tmanager->reloadTexture("res/grassPattern.png", grassPatternId)) {
			err("Couldn't reload grass pattern texture!");
			grassPatternId = -1;
		}
	}

	ImGui::NewLine();

	// -- Ground options ------------------------------------------------------
	ImGui::Text("Ground options");
	ImGui::Separator();

	ImGui::SliderFloat("Cutoff distance", &cutoffDistance, 1.f, 200.f);
	ImGui::SliderFloat("Max distance", &maxDistance, cutoffDistance, 500.f);
	ImGui::SliderFloat("Max factor", &maxFactor, 1.f, 64.f);

	ImGui::End();
}

void Ground::setWindOrigin(const float3 &origin) {
	windData.windOrigin = mul(XMMatrixInverse(nullptr, groundMatrix), origin);
}
