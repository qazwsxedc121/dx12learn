#include "LightingUtil.hlsl"
#include "Constants.hlsl"

VertexOut VS( VertexIn vin )
{
	VertexOut vout;

	float4 posW = mul( float4( vin.PosL, 1.0f ), World);
	vout.PosW = posW.xyz;
	vout.NormalW = mul( vin.NormalL, (float3x3)World );
	vout.PosH = mul(posW, ViewProj);
	vout.Color = vin.Color;
	float4 texC = mul( float4( vin.Tex, 0.0f, 1.0f ), TexTransform);
	vout.Tex = mul(texC, MatTransform).xy;
	return vout;
}