
#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 1
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 1
#endif

#include "LightingUtil.hlsl"

cbuffer cbMaterial : register(b1)
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float  Roughness;
	float4x4 MatTransform;
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4 PS(VertexOut input) : SV_TARGET
{
	return input.color;
}