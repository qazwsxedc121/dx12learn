#include "Waves.h"

#include <algorithm>
#include <cassert>

using namespace DirectX;

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
    NumRows = m;
    NumCols = n;

    VertexCount = m * n;
    TriangleCount = (m - 1) * (n - 1) * 2;

    TimeStep = dt;
    SpatialStep = dx;

    float d = damping * dt + 2.0f;
    float e = (speed * speed) * (dt * dt) / (dx * dx);
    K1 = (damping * dt - 2.0f) / d;
    K2 = (4.0f - 8.0f * e) / d;
    K3 = (2.0f * e) / d;

    PrevSolution.resize(VertexCount);
    CurrSolution.resize(VertexCount);
    Normals.resize(VertexCount);
    TangentX.resize(VertexCount);
    TexCoord.resize(VertexCount);

    // Generate grid vertices in system memory.

    float halfWidth = (n - 1) * dx * 0.5f;
    float halfDepth = (m - 1) * dx * 0.5f;
    for (int i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dx;
        for (int j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            PrevSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            CurrSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            Normals[i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            TangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
            TexCoord[i * n + j] = XMFLOAT2(j / (n - 1.0f), i / (m - 1.0f));
        }
    }
}

Waves::~Waves()
{
}

int Waves::GetRowCount()const
{
    return NumRows;
}

int Waves::GetColumnCount()const
{
    return NumCols;
}

int Waves::GetVertexCount()const
{
    return VertexCount;
}

int Waves::GetTriangleCount()const
{
    return TriangleCount;
}

float Waves::Width()const
{
    return NumCols * SpatialStep;
}

float Waves::Depth()const
{
    return NumRows * SpatialStep;
}

void Waves::Update(float dt)
{
    static float t = 0;

    t += dt;

    if(t >= TimeStep)
    {
        for(int i = 1; i < NumRows-1; ++i)
        {
            for(int j = 1; j < NumCols-1; ++j)
            {
                PrevSolution[i*NumCols+j].y = 
                    K1*PrevSolution[i*NumCols+j].y +
                    K2*CurrSolution[i*NumCols+j].y +
                    K3*(CurrSolution[(i+1)*NumCols+j].y + 
                        CurrSolution[(i-1)*NumCols+j].y + 
                        CurrSolution[i*NumCols+j+1].y + 
                        CurrSolution[i*NumCols+j-1].y);
            }
        }

        std::swap(PrevSolution, CurrSolution);

        t = 0.0f;

        for(int i = 1; i < NumRows - 1; ++i)
        {
            for(int j = 1; j < NumCols - 1; ++j)
            {
                float l = CurrSolution[i*NumCols+j - 1].y;
                float r = CurrSolution[i*NumCols+j + 1].y;
                float t = CurrSolution[(i - 1)*NumCols+j].y;
                float b = CurrSolution[(i + 1)*NumCols+j].y;
                Normals[i*NumCols+j].x = -r + l;
                Normals[i*NumCols+j].y = 2.0f * SpatialStep;
                Normals[i*NumCols+j].z = b - t;

                XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&Normals[i*NumCols+j]));
                XMStoreFloat3(&Normals[i*NumCols+j], n);

                TangentX[i*NumCols+j] = XMFLOAT3(2.0f * SpatialStep, r - l, 0.0f);
                XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&TangentX[i*NumCols+j]));
                XMStoreFloat3(&TangentX[i*NumCols+j], T);
            }
        }
    }
}

void Waves::Disturb(int i, int j, float magnitude)
{
    assert(i > 1 && i < NumRows - 2);
    assert(j > 1 && j < NumCols - 2);

    float halfMag = 0.5f * magnitude;

    CurrSolution[i*NumCols+j].y += magnitude;
    CurrSolution[i*NumCols+j + 1].y += halfMag;
    CurrSolution[i*NumCols+j - 1].y += halfMag;
    CurrSolution[(i + 1)*NumCols+j].y += halfMag;
    CurrSolution[(i - 1)*NumCols+j].y += halfMag;
}