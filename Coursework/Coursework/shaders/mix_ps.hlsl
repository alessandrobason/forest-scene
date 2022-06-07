Texture2D textureA : register(t0);
Texture2D textureB : register(t1);
SamplerState diffSampler : register(s0);

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET{
	float4 colA = textureA.Sample(diffSampler, input.tex);
	float4 colB = textureB.Sample(diffSampler, input.tex);

	return saturate(colA + colB);
}
