
cbuffer cbPerObject : register( b0 )
{
	float4x4 World;
	float4x4 TexTransform;
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

	float4 FogColor;
	float FogStart;
	float FogRange;
	float2 cbPerObjectPad2;

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

Texture2D DiffuseMap : register(t0);
SamplerState DiffuseMapSampler : register(s0);
