#include "MeshCylinderBuilder.h"

MeshBuilder::MeshData CylinderBuilder::BuildCylinder(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount)
{
    MeshData meshData;

    float stackHeight = Height / StackCount;

    float radiusStep = (TopRadius - BottomRadius) / StackCount;

    uint32 ringCount = StackCount + 1;

    for(uint32 i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * Height + i * stackHeight;
        float r = BottomRadius + i * radiusStep;

        float dTheta = 2.0f * XM_PI / SliceCount;
        for(uint32 j = 0; j <= SliceCount; ++j)
        {
            Vertex vertex;

            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.Position = XMFLOAT3(r * c, y, r * s);

            vertex.TexC.x = (float)j / SliceCount;
            vertex.TexC.y = 1.0f - (float)i / StackCount;

            vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

            float dr = BottomRadius - TopRadius;
            XMFLOAT3 bitangent(dr * c, -Height, dr * s);

            XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.Normal, N);

            meshData.Vertices.push_back(vertex);
        }
    }

    uint32 ringVertexCount = SliceCount + 1;

    for(uint32 i = 0; i < StackCount; ++i)
    {
        for(uint32 j = 0; j < SliceCount; ++j)
        {
            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);

            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
            meshData.Indices32.push_back(i * ringVertexCount + j + 1);
        }
    }

    BuildCylinderTopCap(BottomRadius, TopRadius, Height, SliceCount, StackCount, meshData);
    BuildCylinderBottomCap(BottomRadius, TopRadius, Height, SliceCount, StackCount, meshData);

    return meshData;
}

void CylinderBuilder::BuildCylinderTopCap(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount, MeshData &meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();

    float y = 0.5f * Height;
    float dTheta = 2.0f * XM_PI / SliceCount;

    for(uint32 i = 0; i <= SliceCount; ++i)
    {
        float x = TopRadius * cosf(i * dTheta);
        float z = TopRadius * sinf(i * dTheta);

        float u = x / Height + 0.5f;
        float v = z / Height + 0.5f;

        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f));

    for(uint32 i = 0; i < SliceCount; ++i)
    {
        meshData.Indices32.push_back(baseIndex);
        meshData.Indices32.push_back(baseIndex + i + 1);
        meshData.Indices32.push_back(baseIndex + i);
    }
}

void CylinderBuilder::BuildCylinderBottomCap(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount, MeshData &meshData)
{
	uint32 baseIndex = (uint32)meshData.Vertices.size();
	float y = -0.5f*Height;

	float dTheta = 2.0f*XM_PI/SliceCount;
	for(uint32 i = 0; i <= SliceCount; ++i)
	{
		float x = BottomRadius*cosf(i*dTheta);
		float z = BottomRadius*sinf(i*dTheta);

		float u = x/Height + 0.5f;
		float v = z/Height + 0.5f;

		meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
	}

	meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	uint32 centerIndex = (uint32)meshData.Vertices.size()-1;

	for(uint32 i = 0; i < SliceCount; ++i)
	{
		meshData.Indices32.push_back(centerIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i+1);
	}
}
