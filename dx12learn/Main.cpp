#include "stdafx.h"
#include <DirectXPackedVector.h>
#include <iostream>
#include <array>

#include "D3D12App.h"
#include "MathHelper.h"
#include "D3DUtil.h"

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

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstans
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp : public D3D12App
{
public:
	BoxApp(HINSTANCE hInstance) : D3D12App(hInstance)
	{
	}
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp() = default;

	virtual void Render() override;
	virtual void Init() override;
	

protected:
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12DescriptorHeap> CbvHeap;

	std::unique_ptr<UploadBuffer<ObjectConstans>> ObjectCB = nullptr;
	std::unique_ptr<MeshGeometry> BoxGeo = nullptr;

	ComPtr<ID3DBlob> VertexShader;
	ComPtr<ID3DBlob> PixelShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;

	ComPtr<ID3D12PipelineState> PipelineState;

	float Radius = 15.0f;
	float Theta = 1.5f * XM_PI;
	float Phi = XM_PIDIV4;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();

	POINT LastMousePos;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

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
	
	
	BoxApp app(hInstance);
	app.Init();
	app.Main();
	
	return 0;
}

void BoxApp::Render()
{

	if (Device == nullptr)
	{
		return;
	}
	float x = Radius * sinf(Phi) * cosf(Theta);
	float z = Radius * sinf(Phi) * sinf(Theta);
	float y = Radius * cosf(Phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&View, view);

	XMMATRIX world = XMLoadFloat4x4(&World);
	XMMATRIX proj = XMLoadFloat4x4(&Proj);
	XMMATRIX worldViewProj = world * view * proj;

	{
		ObjectConstans objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		ObjectCB->CopyData(0, objConstants);
	}

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

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = BoxGeo->VertexBufferView();
	CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	D3D12_INDEX_BUFFER_VIEW indexBufferView = BoxGeo->IndexBufferView();
	CommandList->IASetIndexBuffer(&indexBufferView);
	CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CommandList->SetGraphicsRootDescriptorTable(0, CbvHeap->GetGPUDescriptorHandleForHeapStart());

	CommandList->DrawIndexedInstanced(BoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

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

void BoxApp::Init()
{
	OutputDebugStringA("Init Start\n");
	D3D12App::Init();

	ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	ThrowIfFailed(CommandList->Close());
	ID3D12CommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	FlushCommandQueue();

	OutputDebugStringA("Init Done\n");
}

void BoxApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, rootParameters, 0, nullptr,
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

void BoxApp::BuildShadersAndInputLayout()
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

void BoxApp::BuildBoxGeometry()
{
	float Color[4] = { 0.1f, 1.0f, 0.5f, 1.0f };
	std::array<Vertex, 8> vertices = {
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Color) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Color) })
	};

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
}

void BoxApp::BuildPSO()
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

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(CbvHeap.GetAddressOf())));
}

void BoxApp::BuildConstantBuffers()
{
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstans>>(Device.Get(), 1, true);
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstans));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = ObjectCB->Resource()->GetGPUVirtualAddress();
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstans));

	Device->CreateConstantBufferView(&cbvDesc, CbvHeap->GetCPUDescriptorHandleForHeapStart());
}
