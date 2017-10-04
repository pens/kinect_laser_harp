#include "stdafx.h"
#include "Main.h"
using namespace DirectX;

__declspec(align(16)) struct CSB {
    float s1, s2, s3, s4, s5, s6, s7, s8;
};

IDXGISwapChain* swapChain;
ID3D11Device* device;
ID3D11DeviceContext* context;
ID3D11RenderTargetView* RTV;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D11ComputeShader* CS;
ID3D11SamplerState* sam;
ID3D11Texture2D* albedoRawTex;
ID3D11Texture2D* albedoTex;
ID3D11Texture2D* depthRawTex;
ID3D11Texture2D* depthTex;
ID3D11ShaderResourceView* albedoRawSRV;
ID3D11ShaderResourceView* depthRawSRV;
ID3D11ShaderResourceView* albedoSRV;
ID3D11ShaderResourceView* depthSRV;
ID3D11UnorderedAccessView* albedoUAV;
ID3D11UnorderedAccessView* depthUAV;
ID3D11Buffer* hitBuff;
ID3D11Buffer* copyBuff;
ID3D11UnorderedAccessView* hitUAV;
IXAudio2* pXAudio2 = 0;
IXAudio2MasteringVoice* pMasterVoice = 0;
IXAudio2SourceVoice* pSourceVoice[8];
ID3D11Buffer* pCSB;
CSB csb;

XAUDIO2_BUFFER buffer[8];

HANDLE depth;
HANDLE albedo;
INuiSensor* sensor;
HWND hwnd;
int feed = 0;

void MapResource(ID3D11Resource* resource, const void* data, size_t size,
                 UINT rowPitch);
void LoadSound(TCHAR* strFileName, WAVEFORMATEXTENSIBLE& wfx,
               XAUDIO2_BUFFER& buffer);

void Main::Init(HWND hwnd) {
    RECT window;
    GetClientRect(hwnd, &window);
    int height = window.bottom;
    int width = window.right;

    DXGI_SWAP_CHAIN_DESC sCDesc{};
    sCDesc.OutputWindow      = hwnd;
    sCDesc.SampleDesc.Count  = 1;
    sCDesc.Windowed          = true;
    sCDesc.BufferCount       = 1;
    sCDesc.BufferDesc.Height = height;
    sCDesc.BufferDesc.Width  = width;
    sCDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sCDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    D3D11CreateDeviceAndSwapChain(0,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  0,
                                  D3D11_CREATE_DEVICE_DEBUG,
                                  0, 0,
                                  D3D11_SDK_VERSION,
                                  &sCDesc,
                                  &swapChain,
                                  &device,
                                  0,
                                  &context);

    D3D11_VIEWPORT viewport{};
    viewport.Height   = (FLOAT)height;
    viewport.Width    = (FLOAT)width;
    viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    ID3D11Texture2D* backBuffTex;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffTex);
    device->CreateRenderTargetView(backBuffTex, 0, &RTV);
    backBuffTex->Release();

    ID3DBlob* blob;
    D3DReadFileToBlob(L"../Debug/VS.cso", &blob);
    device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(),
                               0, &VS);
    D3DReadFileToBlob(L"../Debug/PS.cso", &blob);
    device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(),
                              0, &PS);
    D3DReadFileToBlob(L"../Debug/CS.cso", &blob);
    device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(),
                                0, &CS);
    blob->Release();

    D3D11_SAMPLER_DESC samDesc{};
    samDesc.AddressU       =
        samDesc.AddressV   =
        samDesc.AddressW   = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    device->CreateSamplerState(&samDesc, &sam);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->OMSetRenderTargets(1, &RTV, 0);

    D3D11_TEXTURE2D_DESC texd{}; 
    texd.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    texd.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    texd.Usage            = D3D11_USAGE_DYNAMIC;
    texd.ArraySize        = 1;
    texd.MipLevels        = 1;
    texd.SampleDesc.Count = 1;

    texd.Width  = 1280;
    texd.Height = 960;
    texd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    device->CreateTexture2D(&texd, 0, &albedoRawTex);
    device->CreateShaderResourceView(albedoRawTex, 0, &albedoRawSRV);

    texd.Width = 640;
    texd.Height = 480;
    texd.Format = DXGI_FORMAT_R16G16_UNORM;
    device->CreateTexture2D(&texd, 0, &depthRawTex);
    device->CreateShaderResourceView(depthRawTex, 0, &depthRawSRV);

    texd.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    texd.Usage = D3D11_USAGE_DEFAULT;
    texd.Format = DXGI_FORMAT_R32_FLOAT;
    texd.CPUAccessFlags = 0;
    device->CreateTexture2D(&texd, 0, &depthTex);
    device->CreateUnorderedAccessView(depthTex, 0, &depthUAV);
    device->CreateShaderResourceView(depthTex, 0, &depthSRV);
    texd.Width  = 1280;
    texd.Height = 960;
    texd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    device->CreateTexture2D(&texd, 0, &albedoTex);
    device->CreateUnorderedAccessView(albedoTex, 0, &albedoUAV);
    device->CreateShaderResourceView(albedoTex, 0, &albedoSRV);


    context->CSSetShader(CS, 0, 0);
    context->CSSetShaderResources(0, 1, &depthRawSRV);
    context->CSSetShaderResources(1, 1, &albedoRawSRV);
    context->VSSetShader(VS, 0, 0);
    context->PSSetShader(PS, 0, 0);
    context->PSSetSamplers(0, 1, &sam);

    D3D11_BUFFER_DESC bd{};
    bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bd.ByteWidth = sizeof(int) * 8;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = sizeof(int);
    bd.Usage = D3D11_USAGE_DEFAULT;
    device->CreateBuffer(&bd, 0, &hitBuff);
    bd.BindFlags = 0;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bd.Usage = D3D11_USAGE_STAGING;
    device->CreateBuffer(&bd, 0, &copyBuff);
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd{};
    uavd.Buffer.NumElements = 8;
    uavd.Format = DXGI_FORMAT_UNKNOWN;
    uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    device->CreateUnorderedAccessView(hitBuff, &uavd, &hitUAV);
    context->CSSetUnorderedAccessViews(2, 1, &hitUAV, 0);

    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(CSB);
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    csb.s1 = -3.5f;
    csb.s2 = -6.0f;
    csb.s3 = -12.0f;
    csb.s4 = 0;
    csb.s5 = 12.0f;
    csb.s6 = 6.0f;
    csb.s7 = 3.5f;
    csb.s8 = 0;
    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = &csb;
    device->CreateBuffer(&bd, &srd, &pCSB);
    context->CSSetConstantBuffers(0, 1, &pCSB);

    NuiCreateSensorByIndex(0, &sensor);
    sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH |
                          NUI_INITIALIZE_FLAG_USES_COLOR);
    sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH,
                               NUI_IMAGE_RESOLUTION_640x480,
                               0,
                               NUI_IMAGE_STREAM_FRAME_LIMIT_MAXIMUM,
                               0,
                               &depth);
    sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,
                               NUI_IMAGE_RESOLUTION_1280x960,
                               0,
                               NUI_IMAGE_STREAM_FRAME_LIMIT_MAXIMUM,
                               0,
                               &albedo);
    NuiCameraElevationSetAngle(-4);

    XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    pXAudio2->CreateMasteringVoice(&pMasterVoice);
    WAVEFORMATEXTENSIBLE wfx[8];
    LoadSound(_TEXT("../Debug/1.wav"), wfx[0], buffer[0]);
    LoadSound(_TEXT("../Debug/2.wav"), wfx[1], buffer[1]);
    LoadSound(_TEXT("../Debug/3.wav"), wfx[2], buffer[2]);
    LoadSound(_TEXT("../Debug/4.wav"), wfx[3], buffer[3]);
    LoadSound(_TEXT("../Debug/5.wav"), wfx[4], buffer[4]);
    LoadSound(_TEXT("../Debug/6.wav"), wfx[5], buffer[5]);
    LoadSound(_TEXT("../Debug/7.wav"), wfx[6], buffer[6]);
    for (int i = 0; i < 7; ++i) {
        pXAudio2->CreateSourceVoice(&pSourceVoice[i], (WAVEFORMATEX*)&wfx[i]);
    }
}

void Main::Run() {
    NUI_IMAGE_FRAME frame;
    BOOL nearMode;
    INuiFrameTexture* frameTexture;
    NUI_LOCKED_RECT rect;
    ID3D11ShaderResourceView* nullSRV = 0;
    ID3D11UnorderedAccessView* nullUAV = 0;

    sensor->NuiImageStreamGetNextFrame(depth, 1000, &frame);
    sensor->NuiImageFrameGetDepthImagePixelFrameTexture(
        depth, &frame, &nearMode, &frameTexture);
    frameTexture->LockRect(0, &rect, 0, 0);
    MapResource(depthRawTex, rect.pBits, rect.size, rect.Pitch);
    frameTexture->UnlockRect(0);
    sensor->NuiImageStreamReleaseFrame(depth, &frame);

    sensor->NuiImageStreamGetNextFrame(albedo, 1000, &frame);
    frame.pFrameTexture->LockRect(0, &rect, 0, 0);
    MapResource(albedoRawTex, rect.pBits, rect.size, rect.Pitch);
    frame.pFrameTexture->UnlockRect(0);
    sensor->NuiImageStreamReleaseFrame(albedo, &frame);

    int clearHits[8] ={0, 0, 0, 0, 0, 0, 0, 0};
    context->UpdateSubresource(hitBuff, 0, 0, clearHits, sizeof(int) * 8, 0);
    context->CSSetUnorderedAccessViews(0, 1, &depthUAV, 0);
    context->CSSetUnorderedAccessViews(1, 1, &albedoUAV, 0);
    context->Dispatch(80, 60, 1);
    context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
    context->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);

    int hit[8];
    D3D11_MAPPED_SUBRESOURCE msr;
    context->CopyResource(copyBuff, hitBuff);
    context->Map(copyBuff, 0, D3D11_MAP_READ, 0, &msr);
    memcpy(hit, msr.pData, sizeof(int) * 8);
    context->Unmap(copyBuff, 0);
    for (int i = 0; i < 7; ++i) {
        if (hit[i] > 0) {
            pSourceVoice[i]->SubmitSourceBuffer(&buffer[i]);
            pSourceVoice[i]->Start(0);
            pSourceVoice[i]->SetVolume(1 - (float)hit[i] / 960);
        } else {
            pSourceVoice[i]->Stop();
        }
    }

    context->PSSetShaderResources(0, 1, &depthSRV);
    context->PSSetShaderResources(1, 1, &albedoSRV);
    context->Draw(6, 0);
    swapChain->Present(0, 0);
    context->PSSetShaderResources(0, 1, &nullSRV);
    context->PSSetShaderResources(1, 1, &nullSRV);
}

void Main::Exit() {
    sensor->NuiShutdown();
}

void MapResource(ID3D11Resource* resource, const void* data, size_t size,
                 UINT rowPitch) {
    D3D11_MAPPED_SUBRESOURCE msr{};
    context->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, data, size);
    msr.RowPitch = rowPitch;
    context->Unmap(resource, 0);
}

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition) {
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK) {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType) {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc) {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;

}

HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset) {
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

void LoadSound(TCHAR* strFileName, WAVEFORMATEXTENSIBLE& wfx, 
               XAUDIO2_BUFFER& buffer) {
    HANDLE hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    DWORD dwChunkSize;
    DWORD dwChunkPosition;
    FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD filetype;
    ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
    FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);
    FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE * pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);
    buffer.AudioBytes = dwChunkSize;
    buffer.pAudioData = pDataBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
}