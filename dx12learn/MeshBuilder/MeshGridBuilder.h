#pragma once
#include "../D3DUtil.h"
#include "MeshBuilder.h"

class GridBuilder : MeshBuilder
{
public:
    MeshData BuildGrid(float Width, float Depth, uint32 m, uint32 n);
};