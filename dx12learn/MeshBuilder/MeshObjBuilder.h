#pragma once
#include "../D3DUtil.h"
#include "MeshBuilder.h"

class MeshObjBuilder : MeshBuilder
{
public:
    MeshData BuildByObjFile(const std::string& filename);
};