// triangle_gs
// Geometry shader that generates a triangle for every vertex.

Texture2D grassBase : register(t0);
SamplerState sampler0 : register(s0);

#define MAX_WAVE 10

cbuffer MatrixBuffer : register(b0) {
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer DefaultBuffer : register(b1) {
    float3 cameraPosition;
    float padding1;
};

cbuffer GrassBuffer : register(b2) {
    float timePassed;
    float3 windOrigin;
    float windSpeed;
    float waveAmplitude;
    float2 padding2;
    matrix spotLightMVP;
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
    float4 color : COLOR;
    float4 worldPos : WORLD_POS;
    float3 viewVector : VIEW_VEC;
    float4 spotViewPos : SPOT_LIGHT_POS;
};

float rand(float2 co) {
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

void addGrass(InputType v1, InputType v2, inout TriangleStream<OutputType> triStream) {
    OutputType output;

    matrix mvp = mul(worldMatrix, mul(viewMatrix, projectionMatrix));

    float grassHeight = 1.0;
    float grassLen = 1.0;

    float4 point1 = v1.position;
    float4 point2 = v2.position;
    float4 norm1 = float4(v1.normal, 0.0);
    float4 norm2 = float4(v2.normal, 0.0);
    float4 color = float4(0.9, 0.9, 0.3, 1.0);
    float rand1 = rand(v1.tex);
    float rand2 = rand(v2.tex);

    // Randomize grassHeight
    grassHeight += rand2;

    // Randomize color
    color.r -= rand1;
    color.g -= rand2 * 0.3;
    color = saturate(color);

    float4 parallel = normalize(point2 - point1);
    float4 tangent = normalize(float4(cross(norm1.xyz, parallel.xyz), 0.0));

    // Randomize rotation by changing first point's position along the tangent
    point1 += tangent * rand1;
    parallel = normalize(point2 - point1);

    // Calculate all the points apart from bottomLeft using the
    // parallel and the tangent to make the height and length 
    // consisent
    float4 bottomLeft = point1;
    float4 bottomRight = bottomLeft + parallel * grassLen;
    float4 topLeft = bottomLeft + norm1 * grassHeight;

    // Move top left corner along the tangent so not all grass patches look
    // like they're pointing up, this makes the grass look more full
    topLeft += tangent * rand2;

    // To calculate wind direction, get the average between the two
    // bottom points, then get its direction relative to the "wind origin"
    // (the monolith position) and normalize it, keep distance saved as 
    // we'll need it for wind wave calculation
    float3 windDir = (bottomLeft.xyz + bottomRight.xyz) / 2;
    windDir = windDir - windOrigin;
    float dist = length(windDir);
    windDir = normalize(windDir);

    // Move the top left corner according to the wind
    topLeft.xyz -= windDir * sin(timePassed * windSpeed + dist / waveAmplitude);
    
    // Use a normal vector parallel to topLeft/bottomLeft to calculate the
    // top right corner
    float4 verParallel = normalize(topLeft - bottomLeft);
    topLeft = bottomLeft + verParallel * grassHeight;
    float4 topRight = topLeft + parallel * grassLen;

    // We calculate the dot product of face normal (tangent) and the 
    // camera-to-bottomLeft vector, if the result is negative we display 
    // the anti-clockwise face, otherwise we render the clockwise face
    float4 horParallel = normalize(bottomLeft - bottomRight);
    
    // The tangent in this case is also the normal to the plane
    tangent = float4(cross(horParallel.xyz, verParallel.xyz), 1.0);

    float value = dot(tangent.xyz, bottomLeft.xyz - cameraPosition);

    if (value < 0) {
        // -- a front ---------------------------------------------------------------------------------------

        output.position = mul(bottomLeft, mvp);
        output.worldPos = mul(bottomLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 1.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(bottomRight, mvp);
        output.worldPos = mul(bottomRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 1.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topLeft, mvp);
        output.worldPos = mul(topLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 0.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        triStream.RestartStrip();

        // -- b front ---------------------------------------------------------------------------------------

        output.position = mul(bottomRight, mvp);
        output.worldPos = mul(bottomRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 1.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topRight, mvp);
        output.worldPos = mul(topRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 0.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topLeft, mvp);
        output.worldPos = mul(topLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 0.0);
        output.normal = mul(tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        triStream.RestartStrip();
    }
    else {
        // -- a back ----------------------------------------------------------------------------------------

        output.position = mul(bottomLeft, mvp);
        output.worldPos = mul(bottomLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 1.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topLeft, mvp);
        output.worldPos = mul(topLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 0.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(bottomRight, mvp);
        output.worldPos = mul(bottomRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 1.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        triStream.RestartStrip();

        // -- b back ----------------------------------------------------------------------------------------

        output.position = mul(bottomRight, mvp);
        output.worldPos = mul(bottomRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 1.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topLeft, mvp);
        output.worldPos = mul(topLeft, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(0.0, 0.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        output.position = mul(topRight, mvp);
        output.worldPos = mul(topRight, worldMatrix);
        output.viewVector = normalize(cameraPosition.xyz - output.worldPos.xyz);
        output.spotViewPos = mul(output.worldPos, spotLightMVP);
        output.tex = float2(1.0, 0.0);
        output.normal = mul(-tangent.xyz, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        output.color = color;
        triStream.Append(output);

        triStream.RestartStrip();
    }
}

[maxvertexcount(36)]
void main(triangle InputType input[3], inout TriangleStream<OutputType> triStream) {
    OutputType output;

    // Check that the triangle isn't too close to the windOrigin
    for (int i = 0; i < 3; ++i) {
        float dist = length(input[i].position.xyz - windOrigin);
        if (dist < 10.f) return;
    }

    float2 uv = (input[0].tex + input[1].tex + input[2].tex) / 3;

    // Render only if the red value of this pixel in the grassPattern texture
    // is not 1 (the pixel isn't white)
    if (grassBase.SampleLevel(sampler0, uv, 0).r < 1.0) {
        addGrass(input[0], input[1], triStream);
        addGrass(input[1], input[2], triStream);
        addGrass(input[2], input[0], triStream);
    }
}