#include "TreeShader.h"

#include "utility.h"

TreeShader::TreeShader(Device *device, HWND hwnd)
	: InstanceShader(device, hwnd) {
	initShader(L"shaders/tree_vs.cso", L"shaders/tree_vs_depth.cso");
}

TreeShader::~TreeShader() {
	RELEASE_IF_NOT_NULL(treeBuffer);
}

void TreeShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &proj, 
	TextureType *texture, 
	float4 color, 
	float3 cameraPos, 
	float timePassed, 
	Light lights[LIGHTS_COUNT], 
	ShadowMap *spotShadow, 
	OmniShadowMap &pointShadow, 
	const float3 &windOrigin, 
	f32 windAmplitude, 
	f32 windSpeed
) {
	DefaultShader::setShaderParameters(
		ctx, world, view, proj,
		texture, color,
		cameraPos, timePassed,
		lights, spotShadow, pointShadow
	);

	auto treePtr = mapBuffer<TreeBufferType>(ctx, treeBuffer);
	treePtr->windOrigin    = windOrigin;
	treePtr->windAmplitude = windAmplitude;
	treePtr->windSpeed     = windSpeed;
	unmapBufferVS(ctx, treeBuffer, 3);
}

void TreeShader::initShader(const wchar_t *vs, const wchar_t *dvs) {
	loadVertexShader(vs);
	loadDepthShader(dvs);
	pixelShader = getDefaultPixelShader(this);

	initDefaultBuffers();
	addDynamicBuffer<TreeBufferType>(&treeBuffer);
	addDiffuseSampler();
	addShadowSampler();
}

void TreeShader::loadVertexShader(const wchar_t *vs) {
	// Create the vertex input layout description.
	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "INSTANCE_POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	BaseShader::loadVertexShader(vs, polygonLayout, ARR_LEN(polygonLayout));
}
