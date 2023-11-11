#include "stdafx.h"
#include <DirectXPackedVector.h>
#include <iostream>
#include <array>

#include "D3D12App.h"
#include "MathHelper.h"
#include "D3DUtil.h"

#include "MeshBuilder/MeshBoxBuilder.h"
#include "MeshBuilder/MeshSphereBuilder.h"
#include "MeshBuilder/MeshCylinderBuilder.h"
#include "MeshBuilder/MeshGridBuilder.h"
#include "MeshBuilder/MeshObjBuilder.h"
#include "Simulation/Waves.h"
#include "FrameResource.h"

#include "ResourceUploadBatch.h"
#include "DDSTextureLoader.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

namespace DX
{

}



class DemoApp : public D3D12App
{
public:
	struct RenderItem
	{
		RenderItem() = default;

		XMFLOAT4X4 World = MathHelper::Identity4x4();
		int NumFramesDirty = DemoApp::NumFrameResources;
		UINT ObjCBIndex = -1;
		Material* Mat = nullptr;
		MeshGeometry* Geo = nullptr;
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;
	};

	struct RenderItemWorldInfo
	{
		std::string ObjName = "";
		XMFLOAT3 WorldPos = { 0.0f, 0.0f, 0.0f };
		XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
		XMFLOAT3 Euler = { 0.0f, 0.0f, 0.0f };

		RenderItemWorldInfo(std::string objName, XMFLOAT3 worldPos) :
			ObjName(objName), WorldPos(worldPos)
		{

		}
	};

	DemoApp(HINSTANCE hInstance) : D3D12App(hInstance)
	{
	}
	DemoApp(const DemoApp& rhs) = delete;
	DemoApp& operator=(const DemoApp& rhs) = delete;
	~DemoApp() = default;
	virtual void Init() override;

	const static int NumFrameResources = 3;
protected:

	virtual void Update(const GameTimer& gt) override;
	virtual void Render(const GameTimer& gt) override;
	virtual void OnResize() override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override
	{
		LastMousePos.x = x;
		LastMousePos.y = y;

		SetCapture(mHWnd);
	}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override
	{
		ReleaseCapture();
	}
	float GetHillsHeight(float x, float z) const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}
	XMFLOAT3 GetHillsNormal(float x, float z) const
	{
		XMFLOAT3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z)
		);
		XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
		XMStoreFloat3(&n, unitNormal);
		return n;
	}


	virtual void OnKeyboardInput(const GameTimer& gt);

protected:
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12DescriptorHeap> CbvHeap;
	ComPtr<ID3D12DescriptorHeap> SrvHeap;

	std::vector<std::unique_ptr<FrameResource>> FrameResources;
	FrameResource* CurrentFrameResource = nullptr;
	int CurrentFrameResourceIndex = 0;

	std::unordered_map<std::string, std::unique_ptr<Texture>> Textures;

	ComPtr<ID3DBlob> VertexShader;
	ComPtr<ID3DBlob> PixelShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;

	std::vector<std::unique_ptr<RenderItem>> AllRitems;
	std::vector<RenderItem*> OpaqueRitems;

	RenderItem* WaveRitem;

	PassConstants MainPassCB;

	UINT PassCbvOffset = 0;

	ComPtr<ID3D12PipelineState> PipelineState;

	float Radius = 15.0f;
	float Theta = 1.5f * XM_PI;
	float Phi = XM_PIDIV4;

	XMFLOAT3 EyePos = { 0.0f, 0.0f, 0.0f };

	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();

	POINT LastMousePos;

	std::unordered_map<std::string, MeshBuilder::MeshData> MeshDataMap;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> GeometriesMap;

	std::unordered_map<std::string, std::unique_ptr<Material>> Materials;

	const CD3DX12_STATIC_SAMPLER_DESC SamplerDesc = {
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	};

	float SunTheta = 1.25f * XM_PI;
	float SunPhi = XM_PIDIV4;
	
	void BuildDescriptorHeaps();
	void BuildFrameResources();
	void BuildConstantBuffers();
	void BuildShaderResourceView();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildLandGeometry();
	void BuildWaveGeometry();
	void BuildPSO();
	void BuildRenderItems();
	void LoadTextures();

	void BuildMaterials();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	void UpdateWave(const GameTimer& gt);
	void UpdateMainPassCB();
	void UpdateObjectCBs();
	void UpdateMaterialCBs(const GameTimer& gt);


	std::unique_ptr<Waves> Wave;
};


std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);
	os << "(" << dest.x << "," << dest.y << "," << dest.z << "," << dest.w << ")";
	return os;
}

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMMATRIX m)
{
	for (int i = 0; i < 4; ++i)
	{
		os << XMVectorGetX(m.r[i]) << "\t";
		os << XMVectorGetY(m.r[i]) << "\t";
		os << XMVectorGetZ(m.r[i]) << "\t";
		os << XMVectorGetW(m.r[i]);
		os << endl;
	}
	return os;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	OutputDebugStringA("WinMain Start\n");
	
	
	DemoApp app(hInstance);
	app.Init();
	app.Main();
	
	return 0;
}

void DemoApp::Render(const GameTimer& gt)
{


	ThrowIfFailed(CommandAllocator->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), PipelineState.Get()));

	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &barrier);

	CommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);

	CommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = DepthStencilView();
	D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView = CurrentBackBufferView();
	CommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

	ID3D12DescriptorHeap* descriptorHeaps[] = {SrvHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CommandList->SetGraphicsRootSignature(RootSignature.Get());

	CommandList->SetGraphicsRootConstantBufferView(1, CurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

	DrawRenderItems(CommandList.Get(), OpaqueRitems);

	D3D12_RESOURCE_BARRIER renderTargetToPresent = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &renderTargetToPresent);

	ThrowIfFailed(CommandList->Close());

	ID3D12CommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	ThrowIfFailed(SwapChain->Present(0, 0));
	CurrentBackBufferIndex = (CurrentBackBufferIndex + 1) % 2;

	FlushCommandQueue();

}

void DemoApp::Init()
{
	OutputDebugStringA("Init Start\n");
	D3D12App::Init();

	ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));

	LoadTextures();
	BuildMaterials();

	BuildBoxGeometry();
	BuildLandGeometry();
	BuildWaveGeometry();

	BuildRenderItems();

	BuildDescriptorHeaps();
	BuildFrameResources();
	BuildShaderResourceView();
	//BuildConstantBuffers();

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSO();

	

	ThrowIfFailed(CommandList->Close());
	ID3D12CommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	FlushCommandQueue();

	OutputDebugStringA("Init Done\n");
}

void DemoApp::OnResize()
{
	D3D12App::OnResize();
	XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&Proj, proj);
}

void DemoApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - LastMousePos.y));

		Theta += dx;
		Phi += dy;

		Phi = MathHelper::Clamp(Phi, 0.1f, XM_PI - 0.1f);
	}
	else if((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.05f * static_cast<float>(x - LastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - LastMousePos.y);

		Radius += dx - dy;

		Radius = MathHelper::Clamp(Radius, 3.0f, 15.0f);
	}

	LastMousePos.x = x;
	LastMousePos.y = y;
}

void DemoApp::BuildRootSignature()
{

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[4];
	rootParameters[0].InitAsConstantBufferView(0);  // per object cbv
	rootParameters[1].InitAsConstantBufferView(1);  // per pass cbv
	rootParameters[2].InitAsConstantBufferView(2);  // per material cbv
	rootParameters[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);  // per material cbv

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
		constexpr(sizeof(rootParameters) / sizeof(CD3DX12_ROOT_PARAMETER)), rootParameters, 1, &SamplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(Device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(RootSignature.GetAddressOf())));

}

void DemoApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;
	VertexShader = D3DUtil::CompileShader(L"Shaders/VertexShader.hlsl", nullptr, "VS", "vs_5_0");
	PixelShader = D3DUtil::CompileShader(L"Shaders/PixelShader.hlsl", nullptr, "PS", "ps_5_0");

	InputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, 24,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 24,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void DemoApp::BuildBoxGeometry()
{

	MeshBuilder::MeshData boxData = BoxBuilder().BuildBox(1.0f, 1.5f, 1.5f, 3);
	MeshBuilder::MeshData sphereData = SphereBuilder().BuildSphere(0.5f, 20, 20);
	MeshBuilder::MeshData cylinderData = CylinderBuilder().BuildCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	MeshBuilder::MeshData gridData = GridBuilder().BuildGrid(20.0f, 30.0f, 60, 40);
	MeshBuilder::MeshData teapotData = MeshObjBuilder().BuildByObjFile("Assets/Model/teapot.wobj");

	MeshDataMap["box"] = boxData;
	MeshDataMap["sphere"] = sphereData;
	MeshDataMap["cylinder"] = cylinderData;
	MeshDataMap["grid"] = gridData;
	MeshDataMap["teapot"] = teapotData;

	std::unordered_map<std::string, SubmeshGeometry> submeshGeometries;
	UINT vertexOffset = 0;
	UINT indexOffset = 0;
	UINT totalVertexCount = 0;
	for (const auto& pair : MeshDataMap)
	{
		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)pair.second.Indices32.size();
		submesh.StartIndexLocation = indexOffset;
		submesh.BaseVertexLocation = vertexOffset;
		submeshGeometries[pair.first] = submesh;

		vertexOffset += (UINT)pair.second.Vertices.size();
		indexOffset += (UINT)pair.second.Indices32.size();
		totalVertexCount += (UINT)pair.second.Vertices.size();
	}

	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<XMVECTORF32> colors = { Colors::Red, Colors::Green, Colors::Blue, Colors::Yellow, Colors::Orange, Colors::Purple, Colors::White, Colors::Black };
	UINT k = 0;
	UINT i = 0;
	for(const auto& meshDataPair: MeshDataMap)
	{
		for(size_t j = 0; j < meshDataPair.second.Vertices.size(); ++j, ++k)
		{
			vertices[k].Pos = meshDataPair.second.Vertices[j].Position;
			vertices[k].Normal = meshDataPair.second.Vertices[j].Normal;
			vertices[k].Color = XMFLOAT4(colors[i % colors.size()]);
			vertices[k].TexC = meshDataPair.second.Vertices[j].TexC;
		}
		++i;
	}

	std::vector<std::uint16_t> indices;
	for(const auto& meshDataPair: MeshDataMap)
	{
		std::vector<uint16_t> meshIndicesData = meshDataPair.second.GetIndices16();
		indices.insert(indices.end(), std::begin(meshIndicesData), std::end(meshIndicesData));
	}


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> Geometry = std::make_unique<MeshGeometry>();
	Geometry->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, Geometry->VertexBufferCPU.GetAddressOf()));
	CopyMemory(Geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, Geometry->IndexBufferCPU.GetAddressOf()));
	CopyMemory(Geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	Geometry->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(),
		vertices.data(), vbByteSize, Geometry->VertexBufferUploader);

	Geometry->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(),
		indices.data(), ibByteSize, Geometry->IndexBufferUploader);

	Geometry->VertexByteStride = sizeof(Vertex);
	Geometry->VertexBufferByteSize = vbByteSize;
	Geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	Geometry->IndexBufferByteSize = ibByteSize;

	for (const auto& pair : MeshDataMap)
	{
		Geometry->DrawArgs[pair.first] = submeshGeometries[pair.first];
	}

	GeometriesMap["shapeGeo"] = std::move(Geometry);
}

void DemoApp::BuildLandGeometry()
{
	MeshBuilder::MeshData grid = GridBuilder().BuildGrid(160.0f, 160.0f, 60, 60);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for(size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;

		if(vertices[i].Pos.y < -10.0f)
		{
			vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if(vertices[i].Pos.y < 5.0f)
		{
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if(vertices[i].Pos.y < 12.0f)
		{
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if(vertices[i].Pos.y < 20.0f)
		{
			vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->VertexBufferCPU.GetAddressOf()));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	GeometriesMap["landGeo"] = std::move(geo);

}

void DemoApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
	psoDesc.pRootSignature = RootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(VertexShader->GetBufferPointer()),
		VertexShader->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(PixelShader->GetBufferPointer()),
		PixelShader->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = BackBufferFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DepthStencilFormat;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(PipelineState.GetAddressOf())));

}

void DemoApp::BuildRenderItems()
{
	vector<RenderItemWorldInfo> renderItemWorldInfos = {
		RenderItemWorldInfo("box", XMFLOAT3(0.0f, 0.5f, 0.0f)),
		RenderItemWorldInfo("grid", XMFLOAT3(0.0f, 0.0f, 0.0f)),
		RenderItemWorldInfo("teapot", XMFLOAT3(0.0f, 2.5f, 0.0f)),
	};
	for(int i = 0; i < 5; ++i)
	{
		renderItemWorldInfos.push_back(RenderItemWorldInfo("cylinder", XMFLOAT3(-5.0f, 1.5f, -10.0f + i * 5.0f)));
		renderItemWorldInfos.push_back(RenderItemWorldInfo("cylinder", XMFLOAT3(+5.0f, 1.5f, -10.0f + i * 5.0f)));

		renderItemWorldInfos.push_back(RenderItemWorldInfo("sphere", XMFLOAT3(-5.0f, 3.5f, -10.0f + i * 5.0f)));
		renderItemWorldInfos.push_back(RenderItemWorldInfo("sphere", XMFLOAT3(+5.0f, 3.5f, -10.0f + i * 5.0f)));
	}

	for(size_t i = 0; i < renderItemWorldInfos.size(); ++i)
	{
		auto ritem = std::make_unique<RenderItem>();
		XMMATRIX rItemWorld = XMMatrixRotationRollPitchYaw(renderItemWorldInfos[i].Euler.x, renderItemWorldInfos[i].Euler.y, renderItemWorldInfos[i].Euler.z);
		rItemWorld = XMMatrixMultiply(rItemWorld, XMMatrixScaling(renderItemWorldInfos[i].Scale.x, renderItemWorldInfos[i].Scale.y, renderItemWorldInfos[i].Scale.z));
		rItemWorld = XMMatrixMultiply(rItemWorld, XMMatrixTranslation(renderItemWorldInfos[i].WorldPos.x, renderItemWorldInfos[i].WorldPos.y, renderItemWorldInfos[i].WorldPos.z));
		XMStoreFloat4x4(&ritem->World, rItemWorld);
		const std::string& objName = renderItemWorldInfos[i].ObjName;
		ritem->ObjCBIndex = i;
		ritem->Geo = GeometriesMap["shapeGeo"].get();
		ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->IndexCount = ritem->Geo->DrawArgs[objName].IndexCount;
		ritem->StartIndexLocation = ritem->Geo->DrawArgs[objName].StartIndexLocation;
		ritem->BaseVertexLocation = ritem->Geo->DrawArgs[objName].BaseVertexLocation;
		ritem->Mat = Materials["grass"].get();
		AllRitems.push_back(std::move(ritem));
	}

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = renderItemWorldInfos.size();
	gridRitem->Geo = GeometriesMap["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	gridRitem->Mat = Materials["grass"].get();
	AllRitems.push_back(std::move(gridRitem));

	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = renderItemWorldInfos.size() + 1;
	wavesRitem->Geo = GeometriesMap["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["water"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["water"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["water"].BaseVertexLocation;
	wavesRitem->Mat = Materials["water"].get();
	this->WaveRitem = wavesRitem.get();
	AllRitems.push_back(std::move(wavesRitem));


	for(auto& e : AllRitems)
	{
		OpaqueRitems.push_back(e.get());
	}
}

void DemoApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 0;
	water->DiffuseAlbedo = XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	Materials["grass"] = std::move(grass);
	Materials["water"] = std::move(water);
}

void DemoApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &ritems)
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto matBuffer = CurrentFrameResource->MaterialCB->Resource();

	for(size_t i = 0; i < ritems.size(); ++i)
	{
		RenderItem* ri = ritems[i];
		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &vbv);
		cmdList->IASetIndexBuffer(&ibv);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		cmdList->SetGraphicsRootConstantBufferView(0, CurrentFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize);
		cmdList->SetGraphicsRootConstantBufferView(2, matBuffer->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(SrvHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, CbvSrvUavDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(3, tex);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);

	}

}

void DemoApp::UpdateMainPassCB()
{
	XMMATRIX view = XMLoadFloat4x4(&View);
	XMMATRIX proj = XMLoadFloat4x4(&Proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMMATRIX invView = XMMatrixInverse(&viewDet, view);

	XMVECTOR projDet = XMMatrixDeterminant(proj);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);

	auto viewProjDet = XMMatrixDeterminant(viewProj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	XMStoreFloat4x4(&MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&MainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&MainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	MainPassCB.EyePosW = EyePos;
	MainPassCB.RenderTargetSize = { (float)ClientWidth, (float)ClientHeight };
	MainPassCB.InvRenderTargetSize = { 1.0f / ClientWidth, 1.0f / ClientHeight };
	MainPassCB.NearZ = 1.0f;
	MainPassCB.FarZ = 1000.0f;
	MainPassCB.TotalTime = 0.0f;
	MainPassCB.DeltaTime = 0.0f;
	MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, SunTheta, SunPhi);
	XMStoreFloat3(&MainPassCB.Lights[0].Direction, lightDir);
	MainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };


	CurrentFrameResource->PassCB->CopyData(0, MainPassCB);
}

void DemoApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)AllRitems.size();


	UINT numDescriptors = (objCount + 1) * NumFrameResources;

	PassCbvOffset = objCount * NumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(CbvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(SrvHeap.GetAddressOf())));
}

void DemoApp::BuildFrameResources()
{
	for(int i = 0; i < NumFrameResources; ++i)
	{
		FrameResources.push_back(std::make_unique<FrameResource>(Device.Get(), 1, (UINT)AllRitems.size(), 2, 128*128));
	}
}

void DemoApp::BuildConstantBuffers()
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	UINT objCount = (UINT)AllRitems.size();

	for(int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex)
	{
		auto objectCB = FrameResources[frameIndex]->ObjectCB->Resource();
		for(UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
			cbAddress += i * objCBByteSize;

			int heapIndex = frameIndex * objCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, CbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			Device->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	for(int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex)
	{
		auto passCB = FrameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();
		int heapIndex = frameIndex + PassCbvOffset;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, CbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		Device->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void DemoApp::BuildShaderResourceView()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE Descriptor(SrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	for(auto& pair : Textures)
	{
		std::unique_ptr<Texture>& tex = pair.second;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = tex->Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = tex->Resource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		Device->CreateShaderResourceView(tex->Resource.Get(), &srvDesc, Descriptor);
		Descriptor.Offset(1, CbvSrvUavDescriptorSize);
	}
}

void DemoApp::UpdateObjectCBs()
{
	for(auto& ritem : AllRitems)
	{
		if(ritem->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&ritem->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			CurrentFrameResource->ObjectCB->CopyData(ritem->ObjCBIndex, objConstants);

			ritem->NumFramesDirty--;
		}
	}
}

void DemoApp::UpdateMaterialCBs(const GameTimer &gt)
{
	auto currMaterialCB = CurrentFrameResource->MaterialCB.get();
	for(const auto& pair: Materials)
	{
		Material* mat = pair.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void DemoApp::Update(const GameTimer& gt)
{
	float x = Radius * sinf(Phi) * cosf(Theta);
	float z = Radius * sinf(Phi) * sinf(Theta);
	float y = Radius * cosf(Phi);

	{
		float deltaTime = gt.GetDeltaTime();
		auto boxRitem = AllRitems[0].get();
		float boxX = 2.0f * sinf(gt.GetTotalTime());
		float boxZ = 2.0f * cosf(gt.GetTotalTime());
		XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(boxX, 1.0f, boxZ));
		boxRitem->NumFramesDirty = NumFrameResources;
	}

	EyePos = { x, y, z };

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&View, view);

	XMMATRIX proj = XMLoadFloat4x4(&Proj);

	CurrentFrameResourceIndex = (CurrentFrameResourceIndex + 1) % NumFrameResources;
	CurrentFrameResource = FrameResources[CurrentFrameResourceIndex].get();

	// wait until the gpu has completed commands up to this fence point
	if (CurrentFrameResource->Fence != 0 && Fence->GetCompletedValue() < CurrentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(Fence->SetEventOnCompletion(CurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateWave(gt);
	OnKeyboardInput(gt);

	UpdateMainPassCB();
	UpdateObjectCBs();
	UpdateMaterialCBs(gt);
}

void DemoApp::BuildWaveGeometry()
{
	Wave = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	std::vector<std::uint16_t> indices(3 * Wave->GetTriangleCount());

	int m = Wave->GetRowCount();
	int n = Wave->GetColumnCount();
	int k = 0;

	for(int i = 0; i < m - 1; ++i)
	{
		for(int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;
			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;
			k += 6;
		}
	}

	UINT vbByteSize = (UINT)Wave->GetVertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(Device.Get(), CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["water"] = submesh;

	GeometriesMap["waterGeo"] = std::move(geo);

}

void DemoApp::UpdateWave(const GameTimer& gt)
{
	static float t_base = 0.0f;
	if((Timer.GetTotalTime() - t_base)>=0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, Wave->GetRowCount() - 5);
		int j = MathHelper::Rand(4, Wave->GetColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		Wave->Disturb(i, j, r);
	}

	Wave->Update(gt.GetDeltaTime());

	auto currWavesVB = CurrentFrameResource->WavesVB.get();
	for(int i = 0; i < Wave->GetVertexCount(); ++i)
	{
		Vertex v;

		v.Pos = Wave->GetPosition(i);
		v.Color = XMFLOAT4(DirectX::Colors::Blue);
		v.Normal = Wave->GetNormal(i);

		currWavesVB->CopyData(i, v);
	}

	WaveRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void DemoApp::LoadTextures()
{
	auto riverPebbleTexture = std::make_unique<Texture>();
	riverPebbleTexture->Name = "riverPebbleTex";
	riverPebbleTexture->Filename = L"./Textures/ganges_river_pebbles_1k/textures/ganges_river_pebbles_diff_1k.dds";
	DirectX::ResourceUploadBatch UploadBatch(Device.Get());
	UploadBatch.Begin();
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
		Device.Get(),
		UploadBatch,
		riverPebbleTexture->Filename.c_str(),
		riverPebbleTexture->Resource.GetAddressOf()));
	
	Textures[riverPebbleTexture->Name] = std::move(riverPebbleTexture);

	UploadBatch.End(CommandQueue.Get());


}

void DemoApp::OnKeyboardInput(const GameTimer &gt)
{
	const float dt = gt.GetDeltaTime();

	if(GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		SunTheta -= 1.0f * dt;
	}
	
	if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		SunTheta += 1.0f * dt;
	}

	if(GetAsyncKeyState(VK_UP) & 0x8000)
	{
		SunPhi -= 1.0f * dt;
	}

	if(GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		SunPhi += 1.0f * dt;
	}

	SunPhi = MathHelper::Clamp(SunPhi, 0.1f, XM_PIDIV2);
}
