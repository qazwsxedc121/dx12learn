
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

cbuffer cbMaterial : register(b2)
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float  Roughness;
	float4x4 MatTransform;
};

cbuffer cbPass : register(b1)
{
	float4x4 View;
	float4x4 InvView;
	float4x4 Proj;
	float4x4 InvProj;
	float4x4 ViewProj;
	float4x4 InvViewProj;
	float3 EyePosW;
	float cbPerObjectPad1;
	float2 RenderTargetSize;
	float2 InvRenderTargetSize;
	float NearZ;
	float FarZ;
	float TotalTime;
	float DeltaTime;
	float4 AmbientLight;

	Light Lights[16];
}

Texture2D DiffuseMap : register(t0);
SamplerState DiffuseMapSampler : register(s0);

struct VertexOut
{
	float4 position : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float4 color : COLOR;
	float2 Tex : TEXCOORD;
};

float4 PS(VertexOut input) : SV_TARGET
{
	input.NormalW = normalize(input.NormalW);

	float3 toEyeW = normalize(EyePosW - input.PosW);

	float4 DiffuseAlbedoColor = DiffuseMap.Sample(DiffuseMapSampler, input.Tex) * DiffuseAlbedo;

	float4 ambient = AmbientLight * DiffuseAlbedoColor;

	Material mat = { DiffuseAlbedoColor, FresnelR0, Roughness };
	float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
	float4 directLight = ComputeLighting(Lights, mat, input.PosW, input.NormalW, toEyeW, shadowFactor);

	//float4 litColor = ambient;// + directLight;
	float4 litColor = ambient + directLight;

	litColor.a = DiffuseAlbedo.a;

	return litColor;
}