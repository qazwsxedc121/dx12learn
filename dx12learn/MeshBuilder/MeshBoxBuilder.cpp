#include "MeshBoxBuilder.h"

#include <array>

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

bool BoxBuilder::BuildBox(std::unique_ptr<MeshGeometry>& BoxGeo, ComPtr<ID3D12Device> Device, ComPtr<ID3D12GraphicsCommandList> CommandList,
    float Width, float Height, float Depth)
{
    float halfWidth = 0.5f * Width;
    float halfHeight = 0.5f * Height;
    float halfDepth = 0.5f * Depth;

	std::array<Vertex, 8> vertices = {
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

    for(auto& v : vertices)
    {
        v.Pos.x *= halfWidth;
        v.Pos.y *= halfHeight;
        v.Pos.z *= halfDepth;
    }

	std::array<std::uint16_t, 36> indices = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	BoxGeo = std::make_unique<MeshGeometry>();
	BoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, BoxGeo->VertexBufferCPU.GetAddressOf()));
	CopyMemory(BoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, BoxGeo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(BoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	BoxGeo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(),
		vertices.data(), vbByteSize, BoxGeo->VertexBufferUploader);

	BoxGeo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(),
		indices.data(), ibByteSize, BoxGeo->IndexBufferUploader);

	BoxGeo->VertexByteStride = sizeof(Vertex);
	BoxGeo->VertexBufferByteSize = vbByteSize;
	BoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	BoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	BoxGeo->DrawArgs["box"] = submesh;	
	return true;
}