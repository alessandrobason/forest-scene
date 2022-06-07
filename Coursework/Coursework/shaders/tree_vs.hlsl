#define VS
#include "utils.hlsli"

cbuffer TreeBuffer : register(b3) {
	float3 windOrigin;
	float windAmplitude;
	float windSpeed;
	float3 treePadding;
};

struct InputType {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 instancePosition : INSTANCE_POS;
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

	static const float amplitude = 1 / 40.0;
	static const float waveSpeed = 2.f;

	float4 position = input.position;
	float3 normal = input.normal;

	float3 direction = normalize(windOrigin - input.instancePosition);
	float angle = sin(timePassed * windSpeed) * position.y * windAmplitude;

	float3x3 rotx = rotX(angle * dot(direction, float3(0.0, 0.0, -1.0)));
	float3x3 rotz = rotZ(angle * dot(direction, float3(1.0, 0.0, 0.0)));
	float3x3 rot = mul(rotz, rotx);

	position.xyz = mul(rot, position.xyz);
	normal.xyz = mul(rot, normal.xyz);

	position.xyz += input.instancePosition.xyz;

	float4 worldPosition = mul(position, worldMatrix);
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
	output.normal = mul(normal, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);

	output.spotViewPos = mul(worldPosition, spotLightMVP);

	return output;
}