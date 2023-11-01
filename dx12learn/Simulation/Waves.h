#pragma once

#include <vector>

class Waves
{
public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves& rhs) = delete;
    Waves& operator=(const Waves& rhs) = delete;
    ~Waves();

    int GetRowCount()const;
    int GetColumnCount()const;
    int GetVertexCount()const;
    int GetTriangleCount()const;
    float Width()const;
    float Depth()const;

    const DirectX::XMFLOAT3& GetPosition(int i)const { return CurrSolution[i]; }
    const DirectX::XMFLOAT3& GetNormal(int i)const { return Normals[i]; }
    const DirectX::XMFLOAT3& GetTangentX(int i)const { return TangentX[i]; }

    void Update(float dt);
    void Disturb(int i, int j, float magnitude);

private:
    int NumRows = 0;
    int NumCols = 0;

    int VertexCount = 0;
    int TriangleCount = 0;

    float K1 = 0.0f;
    float K2 = 0.0f;
    float K3 = 0.0f;

    float TimeStep = 0.0f;
    float SpatialStep = 0.0f;

    std::vector<DirectX::XMFLOAT3> PrevSolution;
    std::vector<DirectX::XMFLOAT3> CurrSolution;
    std::vector<DirectX::XMFLOAT3> Normals;
    std::vector<DirectX::XMFLOAT3> TangentX;
};