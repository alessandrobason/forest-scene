#define PS
#include "utils.hlsli"

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET{
	float depth = diffTexture.Sample(diffSampler, input.tex).r;
	float near = 0.1f;
	float far = 100.f;
	return depth;
	return (depth + near) / (far - near);
}
