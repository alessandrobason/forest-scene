Texture2D diffTexture : register(t0);
SamplerState diffSampler : register(s0);

static const float PI = 3.14159265f;
static const float weight[20] = { 0.0658, 0.0656, 0.0653, 0.0648, 0.0643, 0.0636, 0.0627, 0.0618, 0.0607, 0.0605, 0.0581, 0.0564, 0.0543, 0.0517, 0.0484, 0.0445, 0.0399, 0.0340, 0.0269, 0.0181 };

cbuffer BlurBuffer : register(b0) {
	float bloomAmount;
	float3 padding;
};

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

float4 getColor(float2 uv) {
	return diffTexture.Sample(diffSampler, uv);
}

float4 main(InputType input) : SV_TARGET {
	uint width, height;
	diffTexture.GetDimensions(width, height);
	float texelW = 1.0 / width;
	
	float4 colour = getColor(input.tex) * weight[0];

	for (int i = 1; i < 20; ++i) {
		colour += getColor(input.tex + float2(texelW * i, 0.0)) * weight[i] * bloomAmount;
		colour += getColor(input.tex - float2(texelW * i, 0.0)) * weight[i] * bloomAmount;
	}

	return saturate(colour);
}
