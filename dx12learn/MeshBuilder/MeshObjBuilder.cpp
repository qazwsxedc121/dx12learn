#include "MeshObjBuilder.h"

#include <array>
#include <fstream>
#include <sstream>

MeshBuilder::MeshData MeshObjBuilder::BuildByObjFile(const std::string &filename)
{
	MeshData meshData;
	// read obj file lines
	std::ifstream fin(filename);

	if(!fin)
	{
		return MeshData();
	}

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texcoords;

	std::string line;
	while(std::getline(fin, line))
	{
		std::istringstream iss(line);
		char trash;

		if(!line.compare(0, 2, "v "))
		{
			iss >> trash;
			XMFLOAT3 pos;
			iss >> pos.x >> pos.y >> pos.z;
			positions.push_back(pos);
		}
		else if(!line.compare(0, 3, "vn "))
		{
			iss >> trash >> trash;
			XMFLOAT3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if(!line.compare(0, 3, "vt "))
		{
			iss >> trash >> trash;
			XMFLOAT2 texcoord;
			iss >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if(!line.compare(0, 2, "f "))
		{
			iss >> trash;
			std::array<uint32, 3> face = {0, 0, 0};
			std::array<uint32, 3> normal = {0, 0, 0};
			std::array<uint32, 3> texcoord = {0, 0, 0};
			for(int i = 0; i < 3; i++)
			{
				iss >> face[i];
				if(iss.peek() == '/')
				{
					iss >> trash;
					if(iss.peek() != '/')
					{
						iss >> texcoord[i];
					}
					iss >> trash;
					iss >> normal[i];
				}
			}
			meshData.Indices32.push_back(face[0]-1);
			meshData.Indices32.push_back(face[1]-1);
			meshData.Indices32.push_back(face[2]-1);
		}
	}
	fin.close();

	meshData.Vertices.resize(positions.size());
	for(uint32 i = 0; i < positions.size(); i++)
	{
		meshData.Vertices[i].Position = positions[i];
		if(i < normals.size())
		{
			meshData.Vertices[i].Normal = normals[i];
		}
		if(i < texcoords.size())
		{
			meshData.Vertices[i].TexC = texcoords[i];
		}
	}
    return meshData;
}
