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
#include "FrameResource.h"

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
		MeshGeometry* Geo = nullptr;
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;
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

protected:
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12DescriptorHeap> CbvHeap;

	std::vector<std::unique_ptr<FrameResource>> FrameResources;
	FrameResource* CurrentFrameResource = nullptr;
	int CurrentFrameResourceIndex = 0;

	std::unique_ptr<MeshGeometry> Geometry = nullptr;

	ComPtr<ID3DBlob> VertexShader;
	ComPtr<ID3DBlob> PixelShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;

	std::vector<std::unique_ptr<RenderItem>> AllRitems;
	std::vector<RenderItem*> OpaqueRitems;

	PassConstants MainPassCB;

	UINT PassCbvOffset = 0;

	ComPtr<ID3D12PipelineState> PipelineState;

	float Radius = 15.0f;
	float Theta = 1.5f * XM_PI;
	float Phi = XM_PIDIV4;

	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();

	POINT LastMousePos;

	vector<MeshBuilder::MeshData> MeshDataList;

	void BuildDescriptorHeaps();
	void BuildFrameResources();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	void UpdateMainPassCB();
	void UpdateObjectCBs();

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

	float clearColor[] = { 0.3f, 0.2f, 0.1f, 1.0f };
	CommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, nullptr);

	CommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = DepthStencilView();
	D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView = CurrentBackBufferView();
	CommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);
	ID3D12DescriptorHeap* descriptorHeaps[] = { CbvHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CommandList->SetGraphicsRootSignature(RootSignature.Get());

	int passCbvIndex = PassCbvOffset + CurrentFrameResourceIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, CbvSrvUavDescriptorSize);
	CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

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

	BuildBoxGeometry();
	BuildRenderItems();

	BuildDescriptorHeaps();
	BuildFrameResources();
	BuildConstantBuffers();

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
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].InitAsDescriptorTable(1, &cbvTable0);
	rootParameters[1].InitAsDescriptorTable(1, &cbvTable1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr,
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void DemoApp::BuildBoxGeometry()
{
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};

	MeshBuilder::MeshData boxData = BoxBuilder().BuildBox(1.0f, 1.5f, 2.0f, 0);
	MeshBuilder::MeshData sphereData = SphereBuilder().BuildSphere(0.7f, 20, 20);
	MeshBuilder::MeshData cylinderData = CylinderBuilder().BuildCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	MeshDataList.push_back(boxData);
	MeshDataList.push_back(sphereData);
	MeshDataList.push_back(cylinderData);

	vector<SubmeshGeometry> submeshGeometries;
	UINT vertexOffset = 0;
	UINT indexOffset = 0;
	UINT totalVertexCount = 0;
	for (MeshBuilder::MeshData& data : MeshDataList)
	{
		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)data.Indices32.size();
		submesh.StartIndexLocation = indexOffset;
		submesh.BaseVertexLocation = vertexOffset;
		submeshGeometries.push_back(submesh);

		vertexOffset += (UINT)data.Vertices.size();
		indexOffset += (UINT)data.Indices32.size();
		totalVertexCount += (UINT)data.Vertices.size();
	}

	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<XMVECTORF32> colors = { Colors::Red, Colors::Green, Colors::Blue, Colors::Yellow, Colors::Orange, Colors::Purple, Colors::White, Colors::Black };
	UINT k = 0;
	for(size_t i = 0; i < MeshDataList.size(); ++i)
	{
		for(size_t j = 0; j < MeshDataList[i].Vertices.size(); ++j, ++k)
		{
			vertices[k].Pos = MeshDataList[i].Vertices[j].Position;
			vertices[k].Color = XMFLOAT4(colors[i % colors.size()]);
		}
	}

	std::vector<std::uint16_t> indices;
	for(size_t i = 0; i < MeshDataList.size(); ++i)
	{
		indices.insert(indices.end(), std::begin(MeshDataList[i].GetIndices16()), std::end(MeshDataList[i].GetIndices16()));
	}


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	Geometry = std::make_unique<MeshGeometry>();
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

	for(size_t i = 0; i < submeshGeometries.size(); ++i)
	{
		Geometry->DrawArgs["shape_" + std::to_string(i)] = submeshGeometries[i];
	}
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

	for(size_t i = 0; i < MeshDataList.size(); ++i)
	{
		auto ritem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&ritem->World, XMMatrixTranslation(0.0f, (float)i * 3, 0.0f));
		ritem->ObjCBIndex = i;
		ritem->Geo = Geometry.get();
		ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->IndexCount = ritem->Geo->DrawArgs["shape_" + std::to_string(i)].IndexCount;
		ritem->StartIndexLocation = ritem->Geo->DrawArgs["shape_" + std::to_string(i)].StartIndexLocation;
		ritem->BaseVertexLocation = ritem->Geo->DrawArgs["shape_" + std::to_string(i)].BaseVertexLocation;
		AllRitems.push_back(std::move(ritem));
	}

	for(auto& e : AllRitems)
	{
		OpaqueRitems.push_back(e.get());
	}
}

void DemoApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &ritems)
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	for(size_t i = 0; i < ritems.size(); ++i)
	{
		RenderItem* ri = ritems[i];
		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &vbv);
		cmdList->IASetIndexBuffer(&ibv);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		UINT cbvIndex = CurrentFrameResourceIndex * (UINT)AllRitems.size() + ri->ObjCBIndex;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(CbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, CbvSrvUavDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

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
	MainPassCB.EyePosW = { 0.0f, 0.0f, 0.0f };
	MainPassCB.RenderTargetSize = { (float)ClientWidth, (float)ClientHeight };
	MainPassCB.InvRenderTargetSize = { 1.0f / ClientWidth, 1.0f / ClientHeight };
	MainPassCB.NearZ = 1.0f;
	MainPassCB.FarZ = 1000.0f;
	MainPassCB.TotalTime = 0.0f;
	MainPassCB.DeltaTime = 0.0f;

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
}

void DemoApp::BuildFrameResources()
{
	for(int i = 0; i < NumFrameResources; ++i)
	{
		FrameResources.push_back(std::make_unique<FrameResource>(Device.Get(), 1, (UINT)AllRitems.size(), 1));
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

	UpdateMainPassCB();
	UpdateObjectCBs();
}
