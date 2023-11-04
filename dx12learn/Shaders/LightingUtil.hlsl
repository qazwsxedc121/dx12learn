#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

struct Material
{
    float4 Albedo;
    float3 FresnelR0;
    float Roughness;
};

float CalcAttenuation(float3 position, float3 lightPos, float falloffStart, float falloffEnd)
{
    float dist = distance(position, lightPos);
    float atten = saturate((dist - falloffStart) / (falloffEnd - falloffStart));
    return atten * atten;
}

float3 SchlickFresnel(float3 FresnelR0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = FresnelR0+ (1-FresnelR0) * pow(f0, 5.0f);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material material)
{
    const float m = (1.0f - material.Roughness) * 256.0f;
    float3 halfVec = normalize(lightVec + toEye);

    float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(halfVec, normal), 0.0f), m);
    float3 fresnelFactor = SchlickFresnel(material.FresnelR0, normal, lightVec);
    float3 specAlbedo = fresnelFactor * roughnessFactor;

    specAlbedo = specAlbedo / (1.0f + specAlbedo);

    return (material.Albedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectLighting( Light light, Material material, float3 normal, float3 toEye)
{
    float3 lightVec = -light.Direction;
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = light.Strength * ndotl;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float3 ComputePointLighting( Light light, Material material, float3 position, float3 normal, float3 toEye)
{
    float3 lightVec = light.Position - position;
    float dist = length(lightVec);

    if(dist > light.FalloffEnd)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    lightVec = lightVec / dist;
    float atten = CalcAttenuation(position, light.Position, light.FalloffStart, light.FalloffEnd);
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = light.Strength * ndotl * atten;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float3 ComputeSpotLighting( Light light, Material material, float3 position, float3 normal, float3 toEye)
{
    float3 lightVec = light.Position - position;
    float dist = length(lightVec);

    if(dist > light.FalloffEnd)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    lightVec = lightVec / dist;
    float atten = CalcAttenuation(position, light.Position, light.FalloffStart, light.FalloffEnd);
    float ndotl = max(dot(normal, lightVec), 0.0f);

    float spotFactor = pow(max(dot(-lightVec, light.Direction), 0.0f), light.SpotPower);
    float3 lightStrength = light.Strength * ndotl * atten * spotFactor;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float4 ComputeLighting(Light gLights[MaxLights], Material material,
    float3 position, float3 normal, float3 toEye,
    float3 shadowFactor)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);

    int i = 0;

    #if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += ComputeDirectLighting(gLights[i], material, normal, toEye);
    }
    #endif

    #if(NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLighting(gLights[i], material, position, normal, toEye);
    }
    #endif

    #if(NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLighting(gLights[i], material, position, normal, toEye);
    }
    #endif

    return float4(result * shadowFactor, 1.0f);
}