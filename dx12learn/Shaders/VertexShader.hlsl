#include "LightingUtil.hlsl"

cbuffer cbPerObject : register( b0 )
{
	float4x4 gWorld;
	float4x4 TexTransform;
};

cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 AmbientLight;

	Light Lights[16];
}

cbuffer cbMaterial : register(b2)
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float  Roughness;
	float4x4 MatTransform;
};


struct VertexIn
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float4 Color : COLOR;
	float2 Tex : TEXCOORD;
};

VertexOut VS( VertexIn vin )
{
	VertexOut vout;

	float4 posW = mul( float4( vin.PosL, 1.0f ), gWorld);
	vout.PosW = posW.xyz;
	vout.NormalW = mul( vin.NormalL, (float3x3)gWorld );
	vout.PosH = mul(posW, gViewProj);
	vout.Color = vin.Color;
	float4 texC = mul( float4( vin.Tex, 0.0f, 1.0f ), TexTransform);
	vout.Tex = mul(texC, MatTransform).xy;
	return vout;
}