#include "D3DUtil.h"
#include <fstream>
#include <array>

using Microsoft::WRL::ComPtr;

ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, const void *initData, UINT64 byteSize, ComPtr<ID3D12Resource> &uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = initData;
    subresourceData.RowPitch = byteSize;
    subresourceData.SlicePitch = subresourceData.RowPitch;

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &barrier);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subresourceData);
    D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier2);

    return defaultBuffer;
    
}

ComPtr<ID3DBlob> D3DUtil::LoadBinary(const std::wstring &filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ComPtr<ID3DBlob> D3DUtil::CompileShader(const std::wstring &filename, const D3D_SHADER_MACRO *defines, const std::string &entrypoint, const std::string &target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = S_OK;
    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);
    if(errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);
    return byteCode;
}

bool JsonUtil::FromJsonArray(simdjson::ondemand::array& JsonArray, XMFLOAT3& out)
{
    std::array<double, 3> arr = {0.0, 0.0, 0.0};
    int idx = 0;
    for(auto i: JsonArray)
    {
        arr[idx++] = i.get_double();
    }
    out = XMFLOAT3(arr[0], arr[1], arr[2]);
    return true;
}

bool JsonUtil::FromJsonArray(simdjson::ondemand::array& JsonArray, XMFLOAT4& out)
{
    std::array<double, 4> arr = { 0.0, 0.0, 0.0, 0.0 };
    int idx = 0;
    for (auto i : JsonArray)
    {
        arr[idx++] = i.get_double();
    }
    out = XMFLOAT4(arr[0], arr[1], arr[2], arr[3]);
    return true;
}

bool JsonUtil::FromJsonArray(simdjson::ondemand::array& JsonArray, XMFLOAT4X4 &out)
{
    std::array<double, 16> arr = { 0.0, 0.0, 0.0, 0.0 };
    int idx = 0;
    for (auto i : JsonArray)
    {
        arr[idx++] = i.get_double();
    }
    out = XMFLOAT4X4(
        arr[0], arr[1], arr[2], arr[3],
        arr[4], arr[5], arr[6], arr[7],
        arr[8], arr[9], arr[10], arr[11],
        arr[12], arr[13], arr[14], arr[15]);
    return true;
}


bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, int &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        out = val.get_int64();
        return true;
    }
    return false;
}

bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, float &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        out = val.get_double();
        return true;
    }
    return false;
}

bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, std::string_view &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        out = val.get_string();
        return true;
    }
    return false;
}

bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, XMFLOAT3 &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        auto arr = val.get_array().value();
        FromJsonArray(arr, out);
        return true;
    }
    return false;
}

bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, XMFLOAT4 &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        auto arr = val.get_array().value();
        FromJsonArray(arr, out);
        return true;
    }
    return false;
}

bool JsonUtil::ExtractFieldFromObject(simdjson::ondemand::object& JsonObject, const char *key, XMFLOAT4X4 &out)
{
    simdjson::ondemand::value val{};
    if(!JsonObject[key].get(val))
    {
        auto arr = val.get_array().value();
        FromJsonArray(arr, out);
        return true;
    }
    return false;
}