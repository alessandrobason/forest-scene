#define VS
#include "utils.hlsli"

cbuffer TreeBuffer : register(b3) {
	float3 windOrigin;
	float windAmplitude;
	float windSpeed;
	float3 treePadding;
};

struct InputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 instancePosition : INSTANCE_POS;
};

struct OutputType
{
	float4 position : SV_POSITION;
	float4 worldPos : WORLD_POS;
};

OutputType main(InputType input) {
	OutputType output;

	float4 position = input.position;
	float3 normal = input.normal;

	float3 direction = normalize(windOrigin - input.instancePosition);
	float angle = sin(timePassed * windSpeed) * position.y * windAmplitude;

	float3x3 rotx = rotX(angle * dot(direction, float3(0.0, 0.0, -1.0)));
	float3x3 rotz = rotZ(angle * dot(direction, float3(1.0, 0.0, 0.0)));
	float3x3 rot = mul(rotz, rotx);

	position.xyz = mul(rot, position.xyz);
	normal.xyz = mul(rot, normal.xyz);

	position.xyz += input.instancePosition;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.worldPos = mul(position, worldMatrix);
	output.position = mul(output.worldPos, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	return output;
}