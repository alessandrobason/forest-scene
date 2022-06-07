#include "Bloom.h"

#include "utility.h"

// == BLOOM SHADER =======================================================================================================================

BloomShader::BloomShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/bloom_ps.cso");
}

void BloomShader::setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture) {
	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(proj);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	ctx->PSSetShaderResources(0, 1, &texture);
	ctx->PSSetSamplers(0, 1, &sampleState);
}

void BloomShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDiffuseSampler();
}

// == HORIZONTAL BLUR SHADER =============================================================================================================

BlurHorShader::BlurHorShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/blurhor_ps.cso");
}

BlurHorShader::~BlurHorShader() {
	RELEASE_IF_NOT_NULL(blurBuffer);
}

void BlurHorShader::setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture, f32 bloomAmount) {
	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(proj);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	auto blurPtr = mapBuffer<BlurBufferType>(ctx, blurBuffer);
	blurPtr->bloomAmount = bloomAmount;
	blurPtr->padding = { 0, 0, 0 };
	unmapBufferPS(ctx, blurBuffer, 0);

	ctx->PSSetShaderResources(0, 1, &texture);
	ctx->PSSetSamplers(0, 1, &sampleState);
}

void BlurHorShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<BlurBufferType>(&blurBuffer);

	// Don't use the default diffuse sampler as we need the
	// texture to clamp instead of wrap
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

// == VERTICAL BLUR SHADER ===============================================================================================================

BlurVerShader::BlurVerShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/blurver_ps.cso");
}

BlurVerShader::~BlurVerShader() {
	RELEASE_IF_NOT_NULL(blurBuffer);
}

void BlurVerShader::setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &proj, TextureType *texture, f32 bloomAmount) {
	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(proj);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	auto blurPtr = mapBuffer<BlurBufferType>(ctx, blurBuffer);
	blurPtr->bloomAmount = bloomAmount;
	blurPtr->padding = { 0, 0, 0 };
	unmapBufferPS(ctx, blurBuffer, 0);

	ctx->PSSetShaderResources(0, 1, &texture);
	ctx->PSSetSamplers(0, 1, &sampleState);
}

void BlurVerShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDynamicBuffer<BlurBufferType>(&blurBuffer);

	// Don't use the default diffuse sampler as we need the
	// texture to clamp instead of wrap
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

// == MIX SHADER =========================================================================================================================

MixShader::MixShader(Device *device, HWND hwnd)
	: DefaultShader(device, hwnd) {
	initShader(L"shaders/mix_ps.cso");
}

void MixShader::setShaderParameters(
	DeviceContext *ctx, 
	const mat4 &world, const mat4 &view, const mat4 &proj, 
	TextureType *textureA, TextureType *textureB
) {
	// == MATRIX BUFFER =========================
	// Transpose the matrices to prepare them for the shader.
	mat4 tworld = XMMatrixTranspose(world);
	mat4 tview = XMMatrixTranspose(view);
	mat4 tproj = XMMatrixTranspose(proj);
	auto matrixPtr = mapBuffer<MatrixBufferType>(ctx, matrixBuffer);
	matrixPtr->world = tworld;
	matrixPtr->view = tview;
	matrixPtr->projection = tproj;
	unmapBufferVS(ctx, matrixBuffer, 0);

	ctx->PSSetShaderResources(0, 1, &textureA);
	ctx->PSSetShaderResources(1, 1, &textureB);
	ctx->PSSetSamplers(0, 1, &sampleState);
}

void MixShader::initShader(const wchar_t *ps) {
	vertexShader = getBaseVertexShader(this);
	loadPixelShader(ps);

	addDynamicBuffer<MatrixBufferType>(&matrixBuffer);
	addDiffuseSampler();
}

// == BLOOM SHADER =======================================================================================================================

void Bloom::init(Device *device, DeviceContext *deviceContext, int screenWidth, int screenHeight, HWND hwnd) {
	bloomShader    = new BloomShader(device, hwnd);
	blurHorShader  = new BlurHorShader(device, hwnd);
	blurVerShader  = new BlurVerShader(device, hwnd);
	mixShader      = new MixShader(device, hwnd);

	bloomTarget   = new RenderTexture(device, screenWidth, screenHeight, 0.1f, 100.f);
	blurHorTarget = new RenderTexture(device, screenWidth, screenHeight, 0.1f, 100.f);
	blurVerTarget = new RenderTexture(device, screenWidth, screenHeight, 0.1f, 100.f);
	mixTarget     = new RenderTexture(device, screenWidth, screenHeight, 0.1f, 100.f);

	mesh.moveFromMesh(new OrthoMesh(device, deviceContext, screenWidth, screenHeight));
}

Bloom::~Bloom() {
	DELETE_IF_NOT_NULL(bloomShader);
	DELETE_IF_NOT_NULL(blurHorShader);
	DELETE_IF_NOT_NULL(blurVerShader);
	DELETE_IF_NOT_NULL(mixShader);
	DELETE_IF_NOT_NULL(bloomTarget);
	DELETE_IF_NOT_NULL(blurHorTarget);
	DELETE_IF_NOT_NULL(blurVerTarget);
	DELETE_IF_NOT_NULL(mixTarget);
}

TextureType *Bloom::apply(
	DeviceContext *ctx, 
	const mat4 &world, 
	const mat4 &view, 
	const mat4 &proj, 
	TextureType *texture
) {
	ID3D11RenderTargetView *nullRTV = nullptr; 
	TextureType *nullTEX = nullptr; 

	// Clear all the targets
	bloomTarget->clearRenderTarget(ctx,   0.f, 0.f, 0.f, 0.0f);
	blurHorTarget->clearRenderTarget(ctx, 0.f, 0.f, 0.f, 0.0f);
	blurVerTarget->clearRenderTarget(ctx, 0.f, 0.f, 0.f, 0.0f);
	mixTarget->clearRenderTarget(ctx,     0.f, 0.f, 0.f, 0.0f);

	// First extract all the pixels over a certain brightness
	bloomTarget->setRenderTarget(ctx);
	bloomShader->setShaderParameters(ctx, world, view, proj, texture);
	bloomShader->render(ctx, mesh);
	ctx->OMSetRenderTargets(1, &nullRTV, nullptr);

	// Then blur horizontally the texture
	blurHorTarget->setRenderTarget(ctx);
	blurHorShader->setShaderParameters(ctx, world, view, proj, bloomTarget->getShaderResourceView(), bloomAmount);
	blurHorShader->render(ctx, mesh);
	ctx->OMSetRenderTargets(1, &nullRTV, nullptr);

	// Then blur vertically the texture
	blurVerTarget->setRenderTarget(ctx);
	blurVerShader->setShaderParameters(ctx, world, view, proj, blurHorTarget->getShaderResourceView(), bloomAmount);
	blurVerShader->render(ctx, mesh);
	ctx->OMSetRenderTargets(1, &nullRTV, nullptr);

	// Finally, mix the blurred texture with the initial texture
	mixTarget->setRenderTarget(ctx);
	mixShader->setShaderParameters(ctx, world, view, proj, blurVerTarget->getShaderResourceView(), texture);
	mixShader->render(ctx, mesh);
	ctx->OMSetRenderTargets(1, &nullRTV, nullptr);
	ctx->PSSetShaderResources(1, 1, &nullTEX);

	return mixTarget->getShaderResourceView();
}

void Bloom::gui() {
	ImGui::SliderFloat("Bloom amount", &bloomAmount, 0.f, 1.f);
}
