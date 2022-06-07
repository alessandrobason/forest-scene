#pragma once

#include "DXF.h"
#include "types.h"
#include "mmodel.h"
#include "OmniShadowMap.h"

using namespace std;
using namespace DirectX;

struct AttenuationFactor {
	float constant;
	float linear;
	float quadratic;
};

extern AttenuationFactor lightFactors[12];

/* The DefaultShader is a class that any other shader should
 * extend. It has utilities to easily create and map/unmap 
 * buffers. It has four static functions:
 * - getDefaultVertexShader
 * - getBaseVertexShader
 * - getDefaultDepthShader
 * - getDefaultPixelShader
 * They are static so every shader class can share this shaders.
 * For example in post processing the base vertex shader could
 * be used to just pass the data along.
 * Every shader can also have an optional depth vertex shader.
 * This shader should do all the same vertex manipulation but 
 * skip any light calculation.
 */
class DefaultShader : public BaseShader {
protected:
	// Passes data used by most shaders
	struct DefaultBufferType {
		float3 cameraPos;
		float timePassed;
	};

	// Data uset to calculate spotlight's shadows
	struct ShadowBufferType {
		mat4 spotLightMVP;
	};

	// All the data needed to render all the lights
	struct LightBufferType {
		// Data only used by the directional and the spot light
		struct DirSpotData {
			float3 lightDir;
			float spotCutoff;
		} dirSpotData[2];

		// Data shared by all light types
		struct LightData {
			float4 ambient;
			float4 diffuse;
			float4 specular;
			float4 position;
		} sharedLightData[LIGHTS_COUNT];

		// Factors for attenuation calculation, specularPower
		// is also here as it would need to have padding otherwise
		struct Factor {
			float constant;
			float linear;
			float quadratic;
			float specularPower;
		} factor[LIGHTS_COUNT];

		float3 pointLightPos;
		float padding = 0.f;
	};

	// Material data
	struct MaterialBufferType {
		float4 diffuseColor;
	};

	// Data needed for omni-directional shadow mapping
	struct CubemapBufferType {
		mat4 cubeViewMatrix[6];
	};

public:
	DefaultShader(Device *device, HWND hwnd, bool init = false);
	~DefaultShader();
	static void cleanupStaticShaders();
	
	void setShaderParameters(DeviceContext *ctx, const mat4 &world, const mat4 &view, const mat4 &projection, TextureType *texture, float4 color, float3 cameraPos, float timePassed, Light lights[LIGHTS_COUNT], ShadowMap *spotShadow, OmniShadowMap &pointShadow);

	void render(DeviceContext *ctx, MMesh &mesh);

	// Render using default depth vertex shader (disables pixel shader)
	void useDepthShader(bool use);
	// Render using omni depth geometry shader
	void useOmniDepthShader(bool use);

	inline ID3D11VertexShader *getDepthShader() { return vertexDepthShader; }
	inline void setDepthShader(ID3D11VertexShader *newShader) { vertexDepthShader = newShader; }

	bool isDepthShader() { return isDepth; }
	bool isOmniShader() { return isOmni; }

protected:
	void initShader();
	void loadDepthShader(const wchar_t *dvs);
	// Initializes default buffers
	void initDefaultBuffers();
	// Loads diffuse and shadow samplers
	void addDiffuseSampler();
	void addShadowSampler();

	/* default shaders that are loaded once and shared between all shaders */
	static ID3D11VertexShader *getDefaultVertexShader(DefaultShader *ctx);
	static ID3D11VertexShader *getBaseVertexShader(DefaultShader *ctx);
	static ID3D11VertexShader *getDefaultDepthShader(DefaultShader *ctx);
	static ID3D11PixelShader *getDefaultPixelShader(DefaultShader *ctx);

	// Templated function to create dynamic buffer more easily using default options
	template<typename T>
	void addDynamicBuffer(ID3D11Buffer **buf) {
		D3D11_BUFFER_DESC bufDesc{};
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.ByteWidth = sizeof(T);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT res = renderer->CreateBuffer(&bufDesc, NULL, buf);
		res;
	}

	// Templated buffer to easily map buffers
	template<typename T>
	T *mapBuffer(DeviceContext *ctx, ID3D11Buffer *buf) {
		D3D11_MAPPED_SUBRESOURCE mappedResource{};
		HRESULT result = ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		return (T *)mappedResource.pData;
	}

	inline void unmapBufferVS(DeviceContext *ctx, ID3D11Buffer *buf, uint slot) {
		ctx->Unmap(buf, 0);
		ctx->VSSetConstantBuffers(slot, 1, &buf);
	}

	inline void unmapBufferHS(DeviceContext *ctx, ID3D11Buffer *buf, uint slot) {
		ctx->Unmap(buf, 0);
		ctx->HSSetConstantBuffers(slot, 1, &buf);
	}

	inline void unmapBufferDS(DeviceContext *ctx, ID3D11Buffer *buf, uint slot) {
		ctx->Unmap(buf, 0);
		ctx->DSSetConstantBuffers(slot, 1, &buf);
	}

	inline void unmapBufferGS(DeviceContext *ctx, ID3D11Buffer *buf, uint slot) {
		ctx->Unmap(buf, 0);
		ctx->GSSetConstantBuffers(slot, 1, &buf);
	}

	inline void unmapBufferPS(DeviceContext *ctx, ID3D11Buffer *buf, uint slot) {
		ctx->Unmap(buf, 0);
		ctx->PSSetConstantBuffers(slot, 1, &buf);
	}

	static ID3D11VertexShader *defaultDepthShader;

	static ID3D11VertexShader *defaultVertexShader;
	static ID3D11InputLayout *defaultVertexLayout;
	
	static ID3D11VertexShader *baseVertexShader;
	static ID3D11InputLayout *baseVertexLayout;

	static ID3D11PixelShader *defaultPixelShader;

	ID3D11SamplerState *shadowMapSampler = nullptr;
	ID3D11Buffer *matrixBuffer = nullptr;
	ID3D11Buffer *defaultBuffer = nullptr;
	ID3D11Buffer *shadowBuffer = nullptr;
	ID3D11Buffer *lightBuffer = nullptr;
	ID3D11Buffer *materialBuffer = nullptr;
	ID3D11Buffer *cubemapBuffer = nullptr;

	ID3D11VertexShader *vertexDepthShader = nullptr;
	ID3D11GeometryShader *omniDepthGSShader = nullptr;
	bool isDepth = false;
	bool isOmni  = false;
};
