#pragma once
#include "MeshBuilder.h"
class CylinderBuilder :
    public MeshBuilder
{
public:
    MeshData BuildCylinder(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount);
    void BuildCylinderTopCap(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount, MeshData &meshData);
    void BuildCylinderBottomCap(float BottomRadius, float TopRadius, float Height, uint32 SliceCount, uint32 StackCount, MeshData &meshData);
};

