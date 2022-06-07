#define PS
#include "utils.hlsli"

struct InputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float4 worldPosition : WORLD_POS;
	float3 viewVector : VIEW_VEC;
	float4 spotViewPos : SPOT_LIGHT_POS;
};

float4 main(InputType input) : SV_TARGET {
	float4 textureColour = diffTexture.Sample(diffSampler, input.tex);
	if (textureColour.a < 0.5) discard;
	
	float shadowMapBias = 0.005f;

	float4 result = 0;
	float shadow = 0;

	// Directional light
	{
		LightData light = sharedLightData[0];
		DirSpotData dirSpot = dirSpotData[0];
		Factor factors = factor[0];

		float3 dir = normalize(-dirSpot.lightDir);
		float4 lightColour = getLighting(dir, input.normal, light.diffuse);
		float4 specularCol = getSpecular(dir, input.normal, input.viewVector, light.specular, factors.specularPower);
		result = lightColour + light.ambient + specularCol;
	}

	// Spot light
	{
		LightData light = sharedLightData[1];
		DirSpotData dirSpot = dirSpotData[1];
		Factor factors = factor[1];

		if (dirSpot.spotCutoff < 1.f) {
			float3 lightVec = light.position.xyz - input.worldPosition.xyz;
			float attenuation = getAttenuation(length(lightVec), factors.constant, factors.lin, factors.quadratic);
			lightVec = normalize(lightVec);

			float4 lightColour = light.ambient;
			float spotShadow = 0;

			if (isInsideSpotlight(lightVec, dirSpot.lightDir, dirSpot.spotCutoff)) {
				lightColour += getLighting(lightVec, input.normal, light.diffuse);
				lightColour += getSpecular(lightVec, input.normal, input.viewVector, light.specular, factors.specularPower);
				
				spotShadow = getShadow(spotShadowMap, input.spotViewPos, shadowMapBias) * (attenuation * 5);
			}

			result += lightColour * attenuation;

			// -- Spot shadow map -------------------------------------------------------------------
			shadow += spotShadow;
		}
	}

	// Point light
	{
		LightData light = sharedLightData[2];
		Factor factors = factor[2];

		float3 lightVector = light.position.xyz - input.worldPosition.xyz;

		float attenuation = getAttenuation(length(lightVector), factors.constant, factors.lin, factors.quadratic);
		lightVector = normalize(lightVector);
		float4 lightColour = getLighting(lightVector, input.normal, light.diffuse);
		float4 specularCol = getSpecular(lightVector, input.normal, input.viewVector, light.specular, factors.specularPower);

		result += (lightColour + light.ambient + specularCol) * attenuation;

		// -- Point shadow map ------------------------------------------------------------------
		shadow += getPointShadow(input.worldPosition.xyz, 0.0001) * (attenuation * 5);
	}

	result = saturate(result - saturate(shadow));
	return result * textureColour * input.color;
}