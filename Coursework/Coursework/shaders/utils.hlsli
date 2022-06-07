#define LIGHTS_COUNT 3
#define CASCADED_SIZE 3

/* PCF can be turned off for both the shadow maps, with these options:
 * - SHADOW_USE_BLUR turns on PCF for spot shadow maps
 * - SHADOW_BLUR_SAMPLE how many pixels should be looked up on the left/right up/down
 * - POINT_SHADOW_USE_BLUR turns on PCF for omni directional shadow maps
 * - POINT_SHADOW_DISK_RADIUS the radius of the disk used to look up values around the shadow map
 */

// #define SHADOW_USE_BLUR
#define SHADOW_BLUR_SAMPLE 15

// #define POINT_SHADOW_USE_BLUR
#define POINT_SHADOW_DISK_RADIUS 0.5

#ifdef VS
	#define DONT_USE_DEFAULT_PS_BUFFERS
#endif

#ifdef PS
	#define DONT_USE_DEFAULT_VS_BUFFERS
#endif

#ifndef DONT_USE_DEFAULT_VS_BUFFERS

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer DefaultBuffer : register(b1) {
	float3 cameraPosition;
	float timePassed;
};

cbuffer ShadowBuffer : register(b2) {
	matrix spotLightMVP;
};

#endif // DONT_USE_DEFAULT_VS_BUFFERS


#ifndef DONT_USE_DEFAULT_PS_BUFFERS

/* We put lightDir first, as only the directional light needs it,
 * The struct Factor also contains specularPow as it would only be empty
 * padding
 */
cbuffer LightBuffer : register(b0) {
	struct DirSpotData {
		float3 lightDir;
		float spotCutoff;
	} dirSpotData[2];

	struct LightData {
		float4 ambient;
		float4 diffuse;
		float4 specular;
		float4 position;
	} sharedLightData[LIGHTS_COUNT];

	struct Factor {
		float constant;
		float lin;
		float quadratic;
		float specularPower;
	} factor[LIGHTS_COUNT];

	float3 pointLightPos;
	float lightPadding;
};

cbuffer MaterialBuffer : register(b1) {
	float4 matColor;
}

#endif // DONT_USE_DEFAULT_PS_BUFFERS

#ifdef VS
float3x3 rotX(float angle) {
	float s, c;
	sincos(angle, s, c);

	return float3x3(
		1.0, 0.0, 0.0,
		0.0, c, -s,
		0.0, s, c
		);
}

float3x3 rotZ(float angle) {
	float s, c;
	sincos(angle, s, c);

	return float3x3(
		c, -s, 0.0,
		s, c, 0.0,
		0.0, 0.0, 1.0
		);
}
#endif // VS

#ifdef PS

Texture2D diffTexture         : register(t0);
Texture2D spotShadowMap       : register(t1);
TextureCube cubemap           : register(t2);

SamplerState diffSampler      : register(s0);
SamplerState shadowMapSampler : register(s1);

static const float3 cubeOffsetDirection[20] = {
   float3(1,  1,  1), float3( 1, -1,  1), float3(-1, -1,  1), float3(-1,  1,  1),
   float3(1,  1, -1), float3( 1, -1, -1), float3(-1, -1, -1), float3(-1,  1, -1),
   float3(1,  1,  0), float3( 1, -1,  0), float3(-1, -1,  0), float3(-1,  1,  0),
   float3(1,  0,  1), float3(-1,  0,  1), float3( 1,  0, -1), float3(-1,  0, -1),
   float3(0,  1,  1), float3( 0, -1,  1), float3( 0, -1, -1), float3( 0,  1, -1)
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 getLighting(
	float3 lightDirection,
	float3 normal,
	float4 diffuse
) {
	float intensity = saturate(dot(normal, lightDirection));
	return saturate(diffuse * intensity);
}

float4 getSpecular(
	float3 lightDir, 
	float3 normal, 
	float3 viewVector, 
	float4 specularCol, 
	float specularPow
) {
	float3 halfway = normalize(lightDir + viewVector);
	float specularIntensity = pow(max(dot(normal, halfway), 0), specularPow);
	return saturate(specularCol * specularIntensity);
}

float getAttenuation(
	float dist, 
	float constFac, 
	float linFac, 
	float quadFac
) {
	return 1 / (constFac + (linFac * dist) + (quadFac * pow(dist, 2)));
}

bool isInsideSpotlight(
	float3 lightVec, 
	float3 lightDir, 
	float cutoff
) {
	float angle = dot(normalize(-lightDir), lightVec);
	return angle > cutoff;
}

float2 getProjectiveCoords(
	float4 lightViewPosition
) {
	// Calculate the projected texture coordinates.
	float2 projTex = lightViewPosition.xy / lightViewPosition.w;
	projTex *= float2(0.5, -0.5);
	projTex += float2(0.5f, 0.5f);
	return projTex;
}

// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv) {
	return uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f;
}

#define SHADOW_BLUR_SIZE ((SHADOW_BLUR_SAMPLE * 2 + 1) * (SHADOW_BLUR_SAMPLE * 2 + 1))

float getShadow(
	Texture2D shadowMap,
	float4 lightViewPosition,
	float bias
) {
	uint width, height;
	shadowMap.GetDimensions(width, height);
	float2 texelSize = 1.0 / float2(width, height);

	// Calculate the depth from the light.
	float lightDepthValue = (lightViewPosition.z / lightViewPosition.w) - bias;
	
	float2 uv = getProjectiveCoords(lightViewPosition);
	float shadow = 0;

#ifndef SHADOW_USE_BLUR

	if (!hasDepthData(uv)) return 0;
	float depthValue = shadowMap.Sample(shadowMapSampler, uv).r;
	return (lightDepthValue >= depthValue) ? 1 : 0;

#else

	// apply some blurring to the shadow to remove sharp edges
	for (int y = -SHADOW_BLUR_SAMPLE; y <= SHADOW_BLUR_SAMPLE; ++y) {
		for (int x = -SHADOW_BLUR_SAMPLE; x <= SHADOW_BLUR_SAMPLE; ++x) {
			float2 offset = float2(x, y) * texelSize;
			// Sample the shadow map (get depth of geometry)
			float depthValue = shadowMap.SampleLevel(shadowMapSampler, uv + offset, 0).r;
			shadow += (lightDepthValue >= depthValue) ? 1.0 : 0;
		}
	}
	return shadow / SHADOW_BLUR_SIZE;

#endif
}

#undef SHADOW_BLUR_SIZE
#undef SHADOW_BLUR2

float vecToDepth(float3 vec) {
	float3 absVec = abs(vec);
	float localZComp = max(absVec.x, max(absVec.y, absVec.z));

	const float n = 1.0; // near
	const float f = 200; // far
	float normZComp = (f + n) / (f - n) - (f * n * 2) / (f - n) / localZComp;
	return (normZComp + 1.0) * 0.5;
}

float getPointShadow(float3 fragPos, float shadowBias) {
	float3 fragToLight = fragPos - pointLightPos;
	float depth = vecToDepth(fragToLight);
	
#ifndef POINT_SHADOW_USE_BLUR

	float closestDepth = cubemap.Sample(shadowMapSampler, fragToLight).r;
	return (depth - shadowBias) > closestDepth ? 1.0 : 0;

#else

	float shadow = 0.0;

	for (int i = 0; i < 20; ++i) {
		float closestDepth = cubemap.Sample(
			shadowMapSampler,
			fragToLight + cubeOffsetDirection[i] * POINT_SHADOW_DISK_RADIUS
		).r;
		shadow += (depth - shadowBias) > closestDepth ? 1.0 : 0;
	}
	
	return saturate(shadow /= 20);

#endif
}

#endif // PS