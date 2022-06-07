#include "DefaultShader.h"

#include "utility.h"

// this values are taken from https://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation
AttenuationFactor lightFactors[] = {
	{ 1.f, 0.7f, 1.8f },
	{ 1.f, 0.35f, 0.44f },
	{ 1.f, 0.22f, 0.20f },
	{ 1.f, 0.14f, 0.07f },
	{ 1.f, 0.09f, 0.032f },
	{ 1.f, 0.07f, 0.017f },
	{ 1.f, 0.045f, 0.0075f },
	{ 1.f, 0.027f, 0.0028f },
	{ 1.f, 0.022f, 0.0019f },
	{ 1.f, 0.014f, 0.0007f },
	{ 1.f, 0.007f, 0.0002f },
	{ 1.f, 0.0014f, 0.000007f },
};

ID3D11VertexShader *DefaultShader::defaultDepthShader = nullptr;

ID3D11VertexShader *DefaultShader::defaultVertexShader = nullptr;
ID3D11InputLayout *DefaultShader::defaultVertexLayout = nullptr;

ID3D11VertexShader *DefaultShader::baseVertexShader = nullptr;
ID3D11InputLayout *DefaultShader::baseVertexLayout = nullptr;

ID3D11PixelShader *DefaultShader::defaultPixelShader = nullptr;

DefaultShader::DefaultShader(Device *device, HWND hwnd, bool init) 
	: BaseShader(device, hwnd) {
	if (init) {
		initShader();
	}
}

DefaultShader::~DefaultShader() {
	// If we're using a shared shader, don't clenup here
	if (vertexShader == defaultVertexShader) vertexShader = nullptr;
	if (vertexShader == baseVertexShader) vertexShader = nullptr;
	if (vertexDepthShader == defaultDepthShader) vertexDepthShader = nullptr;

	if (layout == defaultVertexLayout) layout = nullptr;
	if (layout == baseVertexLayout) layout = nullptr;

	if (pixelShader == defaultPixelShader) pixelShader = nullptr;

	RELEASE_IF_NOT_NULL(shadowMapSampler);
	RELEASE_IF_NOT_NULL(matrixBuffer);
	RELEASE_IF_NOT_NULL(defaultBuffer);
	RELEASE_IF_NOT_NULL(shadowBuffer);
	RELEASE_IF_NOT_NULL(lightBuffer);
	RELEASE_IF_NOT_NULL(materialBuffer);
	RELEASE_IF_NOT_NULL(cubemapBuffer);
	RELEASE_IF_NOT_NULL(vertexDepthShader);
	RELEASE_IF_NOT_NULL(omniDepthGSShader);
}

void DefaultShader::cleanupStaticShaders() {
	// we don't want to set them to null as we still need to
	// check their pointer when releasing other shaders

	if(defaultVertexShader) defaultVertexShader->Release();
	if(defaultVertexLayout) defaultVertexLayout->Release();

	if(baseVertexShader) baseVertexShader->Release();
	if(baseVertexLayout) baseVertexLayout->Release();

	if(defaultDepthShader) defaultDepthShader->Release();
	if(defaultPixelShader) defaultPixelShader->Release();
}

void DefaultShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &projection, 
	TextureType *texture, 
	float4 color, 
	float3 cameraPos,
	float timePassed,
	Light lights[LIGHTS_COUNT],
	ShadowMap *spotShadow,
	OmniShadowMap &pointShadow
) {
	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(projection);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	// == DEFAULT BUFFER ========================

	auto camPtr = mapBuffer<DefaultBufferType>(ctx, defaultBuffer);
	camPtr->cameraPos = cameraPos;
	camPtr->timePassed = timePassed;
	unmapBufferVS(ctx, defaultBuffer, 1);

	// == SHADOW BUFFER =========================

	auto shadowPtr = mapBuffer<ShadowBufferType>(ctx, shadowBuffer);
	mat4 spotLightView = lights[SPOT_LIGHT].getViewMatrix();
	mat4 spotLightProj = lights[SPOT_LIGHT].getProjectionMatrix();
	shadowPtr->spotLightMVP = XMMatrixTranspose(spotLightView * spotLightProj);
	unmapBufferVS(ctx, shadowBuffer, 2);

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

	if (isOmni) {
		auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
		matrixPtr->world = tworld;
		matrixPtr->view = tview;
		matrixPtr->projection = tproj;
		unmapBufferGS(ctx, matrixBuffer, 0);

		auto cubePtr = mapBuffer<CubemapBufferType>(ctx, cubemapBuffer);
		for (int i = 0; i < 6; ++i) {
			cubePtr->cubeViewMatrix[i] = XMMatrixTranspose(pointShadow.getViewMatrix(i));
		}
		unmapBufferGS(ctx, cubemapBuffer, 1);
	}
}

void DefaultShader::render(DeviceContext *ctx, MMesh &mesh) {
	// == SEND DATA ==============================

	ID3D11Buffer *vbuf = mesh.getVertexBuffer();
	ID3D11Buffer *ibuf = mesh.getIndexBuffer();
	uint stride = sizeof(MMesh::PubVertexType);
	uint offset = 0;

	ctx->IASetVertexBuffers(0, 1, &vbuf, &stride, &offset);
	ctx->IASetIndexBuffer(ibuf, DXGI_FORMAT_R32_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

void DefaultShader::useDepthShader(bool use) {
	isDepth = use;
}

void DefaultShader::useOmniDepthShader(bool use) {
	isOmni  = use;
}

void DefaultShader::initDefaultBuffers() {
	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<DefaultBufferType>(&defaultBuffer);
	addDynamicBuffer<ShadowBufferType>(&shadowBuffer);
	addDynamicBuffer<LightBufferType>(&lightBuffer);
	addDynamicBuffer<MaterialBufferType>(&materialBuffer);
	addDynamicBuffer<CubemapBufferType>(&cubemapBuffer);
}

void DefaultShader::addDiffuseSampler() {
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

void DefaultShader::addShadowSampler() {
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &shadowMapSampler);
}

ID3D11VertexShader *DefaultShader::getDefaultVertexShader(DefaultShader *ctx) {
	assert(ctx);

	if (defaultVertexLayout && !ctx->layout) ctx->layout = defaultVertexLayout;
	if (defaultVertexShader && defaultVertexLayout) return defaultVertexShader;

	// -- Load shader --------------------------------------------------------------------------------------------------------

	ID3DBlob *vertexShaderBuffer = ctx->loadShader(L"shaders/default_vs.cso");

	constexpr D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the vertex shader from the buffer.
	ctx->renderer->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &defaultVertexShader);

	// Create the vertex input layout.
	ctx->renderer->CreateInputLayout(polygonLayout, ARR_LEN(polygonLayout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &defaultVertexLayout);

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	assert(defaultVertexShader && defaultVertexLayout);

	ctx->layout = defaultVertexLayout;
	return defaultVertexShader;
}

ID3D11VertexShader *DefaultShader::getBaseVertexShader(DefaultShader *ctx) {
	assert(ctx);

	if (baseVertexLayout && !ctx->layout) ctx->layout = baseVertexLayout;
	if (baseVertexShader && baseVertexLayout) return baseVertexShader;

	// -- Load shader --------------------------------------------------------------------------------------------------------

	ID3DBlob *vertexShaderBuffer = ctx->loadShader(L"shaders/base_vs.cso");

	constexpr D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the vertex shader from the buffer.
	ctx->renderer->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &baseVertexShader);

	// Create the vertex input layout.
	ctx->renderer->CreateInputLayout(polygonLayout, ARR_LEN(polygonLayout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &baseVertexLayout);

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	assert(baseVertexShader && baseVertexLayout);

	ctx->layout = baseVertexLayout;
	return baseVertexShader;
}

ID3D11VertexShader *DefaultShader::getDefaultDepthShader(DefaultShader *ctx) {
	if (defaultDepthShader) return defaultDepthShader;

	assert(ctx);

	// -- Load shader --------------------------------------------------------------------------------------------------------

	ID3DBlob *vertexShaderBuffer = ctx->loadShader(L"shaders/default_vs_depth.cso");

	// Create the vertex shader from the buffer.
	ctx->renderer->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &defaultDepthShader);

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	assert(defaultDepthShader);

	return defaultDepthShader;
}

ID3D11PixelShader *DefaultShader::getDefaultPixelShader(DefaultShader *ctx) {
	if (defaultPixelShader) return defaultPixelShader;
	
	assert(ctx);

	// -- Load shader --------------------------------------------------------------------------------------------------------

	ID3DBlob *pixelShaderBuffer = ctx->loadShader(L"shaders/default_ps.cso");
	// Create the pixel shader from the buffer.
	ctx->renderer->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &defaultPixelShader);

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	assert(defaultPixelShader);
	
	return defaultPixelShader;
}

void DefaultShader::initShader() {
	vertexShader = getDefaultVertexShader(this);
	vertexDepthShader = getDefaultDepthShader(this);
	pixelShader = getDefaultPixelShader(this);

	initDefaultBuffers();
	addDiffuseSampler();
	addShadowSampler();
}

void DefaultShader::loadDepthShader(const wchar_t *dvs) {
	ID3DBlob *buf = loadShader(dvs);
	// Create the depth shader from the buffer.
	renderer->CreateVertexShader(buf->GetBufferPointer(), buf->GetBufferSize(), NULL, &vertexDepthShader);
	buf->Release();
	buf = nullptr;

	if (!omniDepthGSShader) {
		ID3DBlob *buf = loadShader(L"shaders/omni_gs.cso");
		renderer->CreateGeometryShader(buf->GetBufferPointer(), buf->GetBufferSize(), NULL, &omniDepthGSShader);
		buf->Release();
		buf = nullptr;
	}
}
