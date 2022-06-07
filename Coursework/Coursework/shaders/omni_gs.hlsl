cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}; 

cbuffer CubemapBuffer : register(b1) {
	matrix cubeViewMatrix[6];
}

struct InputType {
	float4 position : SV_POSITION;
	float4 worldPos : WORLD_POS;
};

struct OutputType {
	float4 position : SV_POSITION;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle InputType input[3], inout TriangleStream<OutputType> cubemapStream) {
	for (int f = 0; f < 6; ++f) {
		OutputType output;
		output.RTIndex = f;
		for (int v = 0; v < 3; ++v) {
			output.position = mul(input[v].worldPos, cubeViewMatrix[f]);
			output.position = mul(output.position, projectionMatrix);
			cubemapStream.Append(output);
		}
		cubemapStream.RestartStrip();
	}
}
