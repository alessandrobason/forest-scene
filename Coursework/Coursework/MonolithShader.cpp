#include "MonolithShader.h"

#include "utility.h"

MonolithShader::MonolithShader(Device *device, HWND hwnd) 
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/monolith_ps.cso");
}

MonolithShader::~MonolithShader() {
	RELEASE_IF_NOT_NULL(monolithBuffer);
}

void MonolithShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &projection,
	const float4 &color
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

	// == MONOLITH BUFFER =======================
	// Transpose the matrices to prepare them for the shader.
	auto monoPtr = mapBuffer<MonolithBufferType>(ctx, monolithBuffer);
	monoPtr->color = color;
	unmapBufferPS(ctx, monolithBuffer, 0);
}

void MonolithShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	vertexDepthShader = getDefaultDepthShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<MonolithBufferType>(&monolithBuffer);
}
