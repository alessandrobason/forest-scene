#include "utils.hlsli"

cbuffer MonolithBuffer : register(b0) {
	float4 color;
};

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 main() : SV_TARGET {
	return float4(color.rgb, 200.0);
}