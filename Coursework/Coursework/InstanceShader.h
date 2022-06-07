#pragma once

#include "DefaultShader.h"
#include "types.h"
#include "mmodel.h"

struct InstanceType {
	float3 position;
};

class InstanceShader : public DefaultShader {
public:
	InstanceShader(Device *device, HWND hwnd, bool init = false);
	~InstanceShader();

	template<typename T>
	void renderInstance(Device *device, DeviceContext *ctx, MMesh &mesh, T *idata, uint icount) {
		renderInstanceInternal(device, ctx, mesh, idata, sizeof(T), icount);
	}

protected:
	void initShader(const wchar_t *vs, const wchar_t *dvs);
	void loadVertexShader(const wchar_t *vs);

	void renderInstanceInternal(Device *device, DeviceContext *ctx, MMesh &mesh, void *idata, size_t itypeSize, uint icount);

	ID3D11Buffer *instanceBuffer = nullptr;
};