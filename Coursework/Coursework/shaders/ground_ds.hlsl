#define VS
#include "utils.hlsli"

struct ConstantOutputType {
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

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

#define INTERPOLATE(var) patch[0].##var * uvwCoord.x + patch[1].##var * uvwCoord.y + patch[2].##var * uvwCoord.z

[domain("tri")]
OutputType main(
    ConstantOutputType input,
    float3 uvwCoord : SV_DomainLocation,
    const OutputPatch<InputType, 3> patch
) {
    OutputType output;

    output.tex = INTERPOLATE(tex);
    output.normal = INTERPOLATE(normal);

    float4 vertexPosition = INTERPOLATE(position);
    float4 worldPos = mul(vertexPosition, worldMatrix);

    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.spotViewPos = mul(worldPos, spotLightMVP);

    output.viewVector = cameraPosition.xyz - worldPos.xyz;
    output.viewVector = normalize(output.viewVector);

    return output;
}
