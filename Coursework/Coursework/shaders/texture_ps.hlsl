#define PS
#include "utils.hlsli"

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET{
	return diffTexture.Sample(diffSampler, input.tex);
}
