#pragma once
#include "D3DUtil.h"

using Microsoft::WRL::ComPtr;

class D3D12App
{

public:
	D3D12App(HINSTANCE hInstance);
	virtual void Init();
    void Main();

	virtual void Render();
	static D3D12App* Get();
protected:

	static D3D12App* AppInstance;

	HINSTANCE HInstance;
	ComPtr<IDXGIFactory4> Factory;
	ComPtr<IDXGISwapChain3> SwapChain;
	ComPtr<ID3D12Device> Device;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12Fence1> Fence;

	ComPtr<ID3D12GraphicsCommandList1> CommandList;
	ComPtr<ID3D12Resource> SwapChainBuffer[2];
	ComPtr<ID3D12Resource> DepthStencilBuffer;


	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;

	UINT RtvDescriptorSize;
	UINT DsvDescriptorSize;
	UINT CbvSrvUavDescriptorSize;
	ComPtr<ID3D12DescriptorHeap> RtvHeap;
	ComPtr<ID3D12DescriptorHeap> DsvHeap;
	
	UINT64 CurrentFence;

	UINT CurrentBackBufferIndex;

	void InitFactory();
	void InitDevice();
	void InitSwapChain();
    void InitCommandQueue();
	void InitDescriptorHeap();
	void OnResize();
	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;


	HWND mHWnd;
	static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void InitWindow();
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	int ClientWidth = 1280;
	int ClientHeight = 720;
    bool Minimized = false;
    bool Maximized = false;
    bool Paused = false;
	bool Resizing = false;

	DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
