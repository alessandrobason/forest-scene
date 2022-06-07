Texture2D diffTexture : register(t0);
SamplerState diffSampler : register(s0);

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET{
	float threshold = 3.0;

	float4 colour = diffTexture.Sample(diffSampler, input.tex);
	
	return colour.a > 1.5 ? colour : 0;
}
