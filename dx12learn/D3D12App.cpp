#include "D3D12App.h"

D3D12App* D3D12App::AppInstance = nullptr;

D3D12App::D3D12App(HINSTANCE hInstance)
    :HInstance(hInstance)
{
    AppInstance = this;
}

void D3D12App::Init()
{
    InitWindow();
    InitFactory();
    InitDevice();
    InitCommandQueue();
    InitSwapChain();
    InitDescriptorHeap();
    OnResize();
}

void D3D12App::InitFactory()
{
    UINT dxgiFactoryFlags = 0;

//#if defined(_DEBUG)
#if 0
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));
}

void D3D12App::InitDevice()
{

    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device)));

    ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
}

void D3D12App::InitCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));
    ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));
    ThrowIfFailed(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));
    CommandList->Close();
}

void D3D12App::InitDescriptorHeap()
{
    RtvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CbvSrvUavDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(RtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(DsvHeap.GetAddressOf())));
}

LRESULT D3D12App::StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    D3D12App* Instance = D3D12App::Get();
    if(Instance != nullptr)
    {
        return Instance->WindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT D3D12App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            return 0;
        }
        case WM_KEYDOWN:
        {
            if(wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_PAINT:
        {

            return 0;
        }
        case WM_SIZE:
        {
            ClientWidth = LOWORD(lParam);
            ClientHeight = HIWORD(lParam);
            if(Device)
            {
                if(wParam == SIZE_MINIMIZED)
                {
                    Minimized = true;
                    Maximized = false;
                    Paused = true;
                }
                else if(wParam == SIZE_MAXIMIZED)
                {
                    Minimized = false;
                    Maximized = true;
                    OnResize();
                }
                else if(wParam == SIZE_RESTORED)
                {
                    if(Minimized)
                    {
                        Minimized = false;
                        OnResize();
                    }
                    else if(Maximized)
                    {
                        Maximized = false;
                        OnResize();
                    }
                    else if(Resizing)
                    {
                        // do nothing
                    }
                    else
                    {
                        OnResize();
                    }
                }
            }
            return 0;
        }
        case WM_ENTERSIZEMOVE:
        {
            Resizing = true;
            Paused = true;
            return 0;
        }
        case WM_EXITSIZEMOVE:
        {
            Resizing = false;
            Paused = false;
            OnResize();
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void D3D12App::InitWindow()
{
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = D3D12App::StaticWindowProc;
    windowClass.hInstance = HInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"D3D12App";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, ClientWidth, ClientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    mHWnd = CreateWindow(
        windowClass.lpszClassName,
        L"D3D12App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        HInstance,
        this
    );
    if(!mHWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return;
    }
    ShowWindow(mHWnd, SW_SHOW);
    UpdateWindow(mHWnd);

}

void D3D12App::OnResize()
{
    assert(Device);
    assert(SwapChain);
    assert(CommandAllocator);

    FlushCommandQueue();

    ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));

    for(int i = 0; i < 2; ++i)
    {
        SwapChainBuffer[i].Reset();
    }
    DepthStencilBuffer.Reset();

    ThrowIfFailed(SwapChain->ResizeBuffers(
        2,
        ClientWidth,
        ClientHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    ));

    CurrentBackBufferIndex = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
    for(UINT i = 0; i < 2; ++i)
    {
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i])));
        Device->CreateRenderTargetView(SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, RtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = ClientWidth;
    depthStencilDesc.Height = ClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    CD3DX12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(Device->CreateCommittedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(DepthStencilBuffer.GetAddressOf())
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
    Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &dsvDesc, DsvHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    CommandList->ResourceBarrier(1, &Barrier);

    ThrowIfFailed(CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();


}

void D3D12App::FlushCommandQueue()
{
    CurrentFence++;

    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), CurrentFence));

    if(Fence->GetCompletedValue() < CurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(Fence->SetEventOnCompletion(CurrentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void D3D12App::Main()
{
    //InitSwapChain();
    MSG msg = {};
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

        }
        else
        {
            Render();
        }
    }

}

void D3D12App::Render()
{
}

D3D12App *D3D12App::Get()
{
    return AppInstance;
}

void D3D12App::InitSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = ClientWidth;
    swapChainDesc.Height = ClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(Factory->CreateSwapChainForHwnd(
        CommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
        mHWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(Factory->MakeWindowAssociation(mHWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&SwapChain));
}