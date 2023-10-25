#include "MeshBuilder.h"

void MeshBuilder::Subdivide(MeshData &meshData)
{
    MeshData inputCopy = meshData;

    meshData.Vertices.resize(0);
    meshData.Indices32.resize(0);

    uint32 numTris = inputCopy.Indices32.size() / 3;
    for(uint32 i = 0; i < numTris; ++i)
    {
        Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
        Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
        Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

        meshData.Vertices.push_back(v0);
        meshData.Vertices.push_back(v1);
        meshData.Vertices.push_back(v2);
        meshData.Vertices.push_back(m0);
        meshData.Vertices.push_back(m1);
        meshData.Vertices.push_back(m2);

        meshData.Indices32.push_back(i * 6 + 0);
        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 5);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 2);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 1);
        meshData.Indices32.push_back(i * 6 + 4);
    }
}

MeshBuilder::Vertex MeshBuilder::MidPoint(const Vertex &v0, const Vertex &v1)
{
    XMVECTOR pos0 = XMLoadFloat3(&v0.Position);
    XMVECTOR pos1 = XMLoadFloat3(&v1.Position);

    XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
    XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

    XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
    XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

    XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
    XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

    XMVECTOR pos = 0.5f * (pos0 + pos1);
    XMVECTOR n = XMVector3Normalize(0.5f * (n0 + n1));
    XMVECTOR tan = XMVector3Normalize(0.5f * (tan0 + tan1));
    XMVECTOR tex = 0.5f * (tex0 + tex1);

    Vertex v;
    XMStoreFloat3(&v.Position, pos);
    XMStoreFloat3(&v.Normal, n);
    XMStoreFloat3(&v.TangentU, tan);
    XMStoreFloat2(&v.TexC, tex);

    return v;
}
