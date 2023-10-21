#pragma once


class D3DUtil
{
    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }
};


template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        IsConstantBuffer(isConstantBuffer)
    {
        ElementByteSize = sizeof(T);

        if (isConstantBuffer)
        {
            ElementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));
        }

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // Upload heap
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(ElementByteSize * elementCount), // Resource description for a buffer
            D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
            nullptr,
            IID_PPV_ARGS(&Buffer)));

        ThrowIfFailed(Buffer->Map(0, nullptr, reinterpret_cast<void**>(&MappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
    }
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (Buffer != nullptr)
            Buffer->Unmap(0, nullptr);

        MappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return Buffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&MappedData[elementIndex * ElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;
    BYTE* MappedData = nullptr;

    UINT ElementByteSize = 0;
    bool IsConstantBuffer = false;
};