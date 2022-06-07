#define VS
#include "utils.hlsli"

struct InputType {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct OutputType {
	float4 position : SV_POSITION;
	float4 worldPos : WORLD_POS;
	//float2 tex : TEXCOORD0;
};

OutputType main(InputType input) {
	OutputType output;

	output.worldPos = mul(input.position, worldMatrix);

	output.position = mul(output.worldPos, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	return output;
}