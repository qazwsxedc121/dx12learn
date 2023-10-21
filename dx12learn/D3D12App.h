#pragma once

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch DirectX API errors
		throw std::exception();
	}
}

using Microsoft::WRL::ComPtr;

class D3D12App
{

public:
	D3D12App(HINSTANCE hInstance);
	void Init();
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
};
