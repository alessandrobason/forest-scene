#include "OmniShadowMap.h"

#include "vec.h"
#include "utility.h"

void OmniShadowMap::init(Device *device, int width, int height) {
	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 6;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	
	device->CreateTexture2D(&texDesc, 0, &cubeTexture);
	assert(cubeTexture);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	device->CreateShaderResourceView(cubeTexture, &srvDesc, &cubemap);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.ArraySize = 6;
	dsvDesc.Texture2DArray.FirstArraySlice = 0;
	device->CreateDepthStencilView(cubeTexture, &dsvDesc, &cubeDSV);

	viewport.Width    = (f32)width;
	viewport.Height   = (f32)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
}

OmniShadowMap::~OmniShadowMap() {
	RELEASE_IF_NOT_NULL(cubeDSV);
	RELEASE_IF_NOT_NULL(cubemap);
	RELEASE_IF_NOT_NULL(renderTarget);
	RELEASE_IF_NOT_NULL(cubeTexture);
}

void OmniShadowMap::bind(DeviceContext *ctx) {
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, &renderTarget, cubeDSV);
	ctx->ClearDepthStencilView(cubeDSV, D3D11_CLEAR_DEPTH, 1.f, 0);
}
