#include "TextureIdManager.h"

#include <d3d11.h>

#include "DTK\include\DDSTextureLoader.h"
#include "DTK\include\WICTextureLoader.h"

#include "tracelog.h"
#include "utility.h"

void TextureIdManager::init(Device *ldevice, DeviceContext *lctx) {
	device = ldevice;
	ctx = lctx;
	addDefaultTexture();
}

TextureIdManager::~TextureIdManager() {
	for (ID3D11Texture2D *ptex : pTextures) {
		RELEASE_IF_NOT_NULL(ptex);
	}

	for (TextureType *tex : textures) {
		RELEASE_IF_NOT_NULL(tex);
	}
}

int TextureIdManager::loadTexture(const std::string &filename) {
	return loadTextureAt(filename, (int)textures.size());
}

int TextureIdManager::loadTexture(void *data, uint size) {
	TextureType *texture = nullptr;
	int index = -1;

	HRESULT result = DirectX::CreateWICTextureFromMemory(
		device, 
		(uint8_t *)data, size, 
		NULL, &texture
	);

	if (FAILED(result)) {
		err("Couldn't load texture, data: <%p>, size: <%u>", data, size);
	}
	else {
		index = (int)textures.size();
		textures.emplace_back(texture);
	}

	return index;
}

int TextureIdManager::loadTexture(PixelData *data, uint width, uint height) {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA initialData{};
	initialData.pSysMem = data;
	initialData.SysMemPitch = sizeof(PixelData) * width * height;
	initialData.SysMemSlicePitch = 0;

	ID3D11Texture2D *texture2d = nullptr;

	HRESULT result = device->CreateTexture2D(&desc, &initialData, &texture2d);
	pTextures.emplace_back(texture2d);

	if (SUCCEEDED(result)) {
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		TextureType *texture = nullptr;
		result = device->CreateShaderResourceView(texture2d, &SRVDesc, &texture);
		textures.emplace_back(texture);
	}
	else {
		err("failed to create texture, pixel data: <%p>, width: <%u>, height: <%u>", data, width, height);
	}

	return 0;
}

int TextureIdManager::loadTextureAt(const std::string &filename, int id) {
	int index = -1;

	std::string extension;
	wchar_t *wFilename = nullptr;
	TextureType *texture = nullptr;
	HRESULT result;

	if (!fileExists(filename.c_str())) {
		err("couldn't find file \"%s\"", filename.c_str());
		goto error;
	}

	auto idx = filename.rfind('.');

	if (idx != std::string::npos) {
		extension = filename.substr(idx + 1);
	}

	wFilename = wstrFromStr(filename.c_str(), filename.size());
	if (!wFilename) goto error;

	if (extension == "dds") {
		result = DirectX::CreateDDSTextureFromFile(
			device, ctx,
			wFilename, NULL,
			&texture, 0
		);
	}
	else {
		result = DirectX::CreateWICTextureFromFile(
			device, ctx,
			wFilename, NULL, 
			&texture, 0
		);
	}

	delete[] wFilename;

	if (FAILED(result)) {
		err("Couldn't load texture %s -> %d", filename.c_str(), result);
		goto error;
	}
	else {
		index = id;
		if (index == textures.size()) {
			textures.emplace_back(texture);
		}
		else {
			textures[index] = texture;
		}
	}

error:
	return index;
}

bool TextureIdManager::reloadTexture(const std::string &filename, int id) {
	RELEASE_IF_NOT_NULL(textures[id]);
	int result = loadTextureAt(filename, id);
	return result == id;
}

TextureType *TextureIdManager::getTexture(int id) {
	// if the index is out of range, return the default texture
	if (id < 0 || id >= textures.size()) return textures[0];
	return textures[id];
}

TextureType *TextureIdManager::operator[](int id) {
	return getTexture(id);
}

void TextureIdManager::addDefaultTexture() {
	static const uint32_t s_pixel = 0xffffffff;
	
	D3D11_SUBRESOURCE_DATA initData{
		&s_pixel,
		sizeof(s_pixel),
		0
	};

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D *texture2d = nullptr;
	HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture2d);
	pTextures.emplace_back(texture2d);

	if (SUCCEEDED(hr)) {
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		TextureType *texture = nullptr;
		hr = device->CreateShaderResourceView(texture2d, &SRVDesc, &texture);
		textures.emplace_back(texture);
	}
}
