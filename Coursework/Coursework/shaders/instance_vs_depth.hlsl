// Light vertex shader

#define VS
#include "utils.hlsli"

struct InputType {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 instancePosition : TEXCOORD1;
};

struct OutputType {
	float4 position : SV_POSITION;
	float4 depthPosition : TEXCOORD0;
};

OutputType main(InputType input) {
	OutputType output;
	// Update the position of the vertices based on the data for this particular instance.

	input.position.xyz += input.instancePosition;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	output.depthPosition = output.position;

	return output;
}