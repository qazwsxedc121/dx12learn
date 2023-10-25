#pragma once
#include "../D3DUtil.h"
class BoxBuilder
{
public:
    static bool BuildBox(std::unique_ptr<MeshGeometry> &BoxGeo, ComPtr<ID3D12Device> Device, ComPtr<ID3D12GraphicsCommandList> CommandList,
        float Width, float Height, float Depth
    );
};