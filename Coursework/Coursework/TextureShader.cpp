#include "TextureShader.h"

TextureShader::TextureShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/texture_ps.cso");
}

void TextureShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &projection, 
	TextureType *texture
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

	// Set shader texture resource in the pixel shader.
	ctx->PSSetShaderResources(0, 1, &texture);
	ctx->PSSetSamplers(0, 1, &sampleState);
}

void TextureShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDiffuseSampler();
}