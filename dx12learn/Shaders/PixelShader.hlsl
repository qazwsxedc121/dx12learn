
#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"
#include "Constants.hlsl"


float4 PS(VertexOut input) : SV_TARGET
{
	float4 DiffuseAlbedoColor = DiffuseMap.Sample(DiffuseMapSampler, input.Tex) * DiffuseAlbedo;

#ifdef ALPHA_TEST
	clip(DiffuseAlbedoColor.a - 0.1f);
#endif

	input.NormalW = normalize(input.NormalW);

	float3 toEyeW = normalize(EyePosW - input.PosW);


	float4 ambient = AmbientLight * DiffuseAlbedoColor;

	Material mat = { DiffuseAlbedoColor, FresnelR0, Roughness };
	float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
	float4 directLight = ComputeLighting(Lights, mat, input.PosW, input.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

#ifdef FOG
	float fogFactor = saturate((input.PosW.z - NearZ) / (FarZ - NearZ));
	litColor = lerp(litColor, FogColor, fogFactor);
#endif

	litColor.a = DiffuseAlbedo.a;

	return litColor;
}