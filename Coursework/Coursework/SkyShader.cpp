#include "SkyShader.h"

#include "utility.h"

SkyShader::SkyShader(Device *device, HWND hwnd) 
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/sky_vs.cso", L"shaders/sky_ps.cso");
}

SkyShader::~SkyShader() {
	RELEASE_IF_NOT_NULL(skyBuffer);
}

void SkyShader::setShaderParameters(
	DeviceContext *ctx,
	const mat4 &world,
	const mat4 &view,
	const mat4 &projection,
	TextureType *texture,
	float3 cameraPos,
	float timePassed,
	Light *light,
	const SkyData &skyData
) {
	// == MATRIX BUFFER =========================
	mat4 tworld, tview, tproj;

	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(world);
	tview = XMMatrixTranspose(view);
	tproj = XMMatrixTranspose(projection);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	// == CAMERA BUFFER =========================

	auto camPtr = mapBuffer<DefaultBufferType>(ctx, defaultBuffer);
	camPtr->cameraPos = cameraPos;
	camPtr->timePassed = timePassed;
	unmapBufferVS(ctx, defaultBuffer, 1);

	// == SKY BUFFER =============================

	auto skyPtr = mapBuffer<SkyData>(ctx, skyBuffer);
	*skyPtr = skyData;
	unmapBufferPS(ctx, skyBuffer, 0);

	// Set shader texture resource in the pixel shader.
	ctx->PSSetShaderResources(0, 1, &texture);
	ctx->PSSetSamplers(0, 1, &sampleState);

}

void SkyShader::initShader(const wchar_t *vs, const wchar_t *ps) {
	loadVertexShader(vs);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<DefaultBufferType>(&defaultBuffer);
	addDynamicBuffer<SkyData>(&skyBuffer);
	addDiffuseSampler();
}
