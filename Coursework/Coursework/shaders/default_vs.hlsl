#define VS
#include "utils.hlsli"

struct InputType {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct OutputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 worldPosition : WORLD_POS;
	float3 viewVector : VIEW_VEC;
	float4 spotViewPos : SPOT_LIGHT_POS;
};

OutputType main(InputType input) {
	OutputType output;

	float4 worldPosition = mul(input.position, worldMatrix);
	
	output.worldPosition = worldPosition.xyz;
	output.viewVector = cameraPosition.xyz - worldPosition.xyz;
	output.viewVector = normalize(output.viewVector);

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = worldPosition;
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Calculate the normal vector against the world matrix only and normalise.
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);

	output.spotViewPos = mul(worldPosition, spotLightMVP);

	return output;
}