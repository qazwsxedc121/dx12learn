#include "stdafx.h"
#include <DirectXPackedVector.h>
#include <iostream>

#include "D3D12App.h"
#include "MathHelper.h"
#include "D3DUtil.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

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
	virtual void Render() override;


protected:
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12DescriptorHeap> CbvHeap;

	std::unique_ptr<UploadBuffer<ObjectConstans>> ObjectCB = nullptr;

	

	float Radius = 15.0f;
	float Theta = 1.5f * XM_PI;
	float Phi = XM_PIDIV4;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();

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
	D3D12App app(hInstance);
	app.Init();
	app.Main();
	return 0;
}

void BoxApp::Render()
{
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

	ObjectConstans objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	ObjectCB->CopyData(0, objConstants);

}
