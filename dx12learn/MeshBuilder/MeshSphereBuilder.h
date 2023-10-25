#pragma once
#include "../D3DUtil.h"
#include "MeshBuilder.h"

class SphereBuilder : MeshBuilder
{
public:
    MeshData BuildSphere(float radius, uint32 sliceCount, uint32 stackCount);
};