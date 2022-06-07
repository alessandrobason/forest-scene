#pragma once
#include "d3d.h"

using namespace DirectX;

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, int mWidth, int mHeight);
	~ShadowMap();

	void BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* dc);

	void setDepthMapSRV(ID3D11ShaderResourceView *newDepthMap) { mDepthMapSRV = newDepthMap; };
	ID3D11ShaderResourceView* getDepthMapSRV() { return mDepthMapSRV; };

	void setDepthMapDSV(ID3D11DepthStencilView *newDepthMap) { mDepthMapDSV = newDepthMap; };
	ID3D11DepthStencilView * getDepthMapDSV() { return mDepthMapDSV; };

private:
	ID3D11DepthStencilView* mDepthMapDSV;
	ID3D11ShaderResourceView* mDepthMapSRV;
	D3D11_VIEWPORT viewport;
	ID3D11RenderTargetView* renderTargets;
	ID3D11Texture2D* depthMap;
};