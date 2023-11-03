#pragma once
#include <vector>
#include "../MathHelper.h"

class MeshBuilder
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    struct Vertex
    {
        Vertex() = default;
        Vertex(
            const XMFLOAT3& p,
            const XMFLOAT3& n,
            const XMFLOAT3& t,
            const XMFLOAT2& uv) :
            Position(p),
            Normal(n),
            TangentU(t),
            TexC(uv)
        {}

        Vertex(
			float px, float py, float pz, 
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v) : 
            Position(px,py,pz), 
            Normal(nx,ny,nz),
			TangentU(tx, ty, tz), 
            TexC(u,v)
        {}

        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT3 TangentU;
        XMFLOAT2 TexC;
    };

    struct MeshData
    {
    public:
        std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;

        std::vector<uint16>& GetIndices16() const
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); ++i)
                    mIndices16[i] = static_cast<uint16>(Indices32[i]);
            }

            return mIndices16;
        }
    private:
        mutable std::vector<uint16> mIndices16;
    };

protected:
    void Subdivide(MeshData& meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
    
};