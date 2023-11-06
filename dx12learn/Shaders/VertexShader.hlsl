#include "LightingUtil.hlsl"

cbuffer cbPerObject : register( b0 )
{
	float4x4 gWorld;
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

struct VertexIn
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float4 Color : COLOR;
};

VertexOut VS( VertexIn vin )
{
	VertexOut vout;

	float4 posW = mul( float4( vin.PosL, 1.0f ), gWorld);
	vout.PosW = posW.xyz;
	vout.NormalW = mul( vin.NormalL, (float3x3)gWorld );
	vout.PosH = mul(posW, gViewProj);
	vout.Color = vin.Color;
	return vout;
}