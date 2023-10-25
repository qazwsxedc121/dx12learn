#pragma once
#include "../D3DUtil.h"
#include "MeshBuilder.h"

class BoxBuilder : MeshBuilder
{
public:
    MeshData BuildBox(float Width, float Height, float Depth, uint32 NumSubdivisions);
};