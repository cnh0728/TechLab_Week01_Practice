#include "URenderer.h"

#include "UObject.h"

/** Renderer를 초기화 합니다. */
void URenderer::Create(HWND hWindow)
{
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
    CreateRasterizerState();
}

/** Renderer에 사용된 모든 리소스를 해제합니다. */
void URenderer::Release()
{
    ReleaseRasterizerState();

    // 렌더 타겟을 초기화
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

void URenderer::CreateShader()
{
    /**
     * 컴파일된 셰이더의 바이트코드를 저장할 변수 (ID3DBlob)
     *
     * 범용 메모리 버퍼를 나타내는 형식
     *   - 여기서는 shader object bytecode를 담기위해 쓰임
     * 다음 두 메서드를 제공한다.
     *   - LPVOID GetBufferPointer
     *     - 버퍼를 가리키는 void* 포인터를 돌려준다.
     *   - SIZE_T GetBufferSize
     *     - 버퍼의 크기(바이트 갯수)를 돌려준다
     */
    ID3DBlob* VertexShaderCSO;
    ID3DBlob* PixelShaderCSO;
    ID3DBlob* UIDShaderCSO;
    
    // 셰이더 컴파일 및 생성
    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderCSO, nullptr);
    Device->CreateVertexShader(VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderCSO, nullptr);
    Device->CreatePixelShader(PixelShaderCSO->GetBufferPointer(), PixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

    D3DCompileFromFile(L"Shaders/ShaderW0.hlsl", nullptr, nullptr, "UUIDPS", "ps_5_0", 0, 0, &UIDShaderCSO, nullptr);
    Device->CreatePixelShader(UIDShaderCSO->GetBufferPointer(), UIDShaderCSO->GetBufferSize(), nullptr, &UIDPixelShader);

    // 입력 레이아웃 정의 및 생성
    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Device->CreateInputLayout(Layout, ARRAYSIZE(Layout), VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

    VertexShaderCSO->Release();
    PixelShaderCSO->Release();
    UIDShaderCSO->Release();

    // 정점 하나의 크기를 설정 (바이트 단위)
    Stride = sizeof(FVertexSimple);
}
void URenderer::ReleaseShader()
{
    if (SimpleInputLayout)
    {
        SimpleInputLayout->Release();
        SimpleInputLayout = nullptr;
    }

    if (SimplePixelShader)
    {
        SimplePixelShader->Release();
        SimplePixelShader = nullptr;
    }

    if (SimpleVertexShader)
    {
        SimpleVertexShader->Release();
        SimpleVertexShader = nullptr;
    }

    if (UIDPixelShader)
    {
        UIDPixelShader->Release();
        UIDPixelShader = nullptr;
    }
}

void URenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC ConstantBufferDesc = {};
    ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;                        // 매 프레임 CPU에서 업데이트 하기 위해
    ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;             // 상수 버퍼로 설정
    ConstantBufferDesc.ByteWidth = sizeof(FUUIDConstants) + 0xf & 0xfffffff0;  // 16byte의 배수로 올림
    ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;            // CPU에서 쓰기 접근이 가능하게 설정

    Device->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantWorldBuffer);
    Device->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantUUIDBuffer);
}
void URenderer::ReleaseConstantBuffer()
{
    if (ConstantWorldBuffer)
    {
        ConstantWorldBuffer->Release();
        ConstantWorldBuffer = nullptr;
    }
    if (ConstantUUIDBuffer)
    {
        ConstantUUIDBuffer->Release();
        ConstantUUIDBuffer = nullptr;
    }
}

void URenderer::CreatePickingShader()
{
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    Device->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState);

    D3D11_RENDER_TARGET_BLEND_DESC rtbd;
    ZeroMemory(&rtbd, sizeof(rtbd));
    rtbd.RenderTargetWriteMask = 0; // 모든 색상 채널 쓰기 비활성화

    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0] = rtbd;

    Device->CreateBlendState(&blendDesc, &BlendState);
}

/** 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력 */
void URenderer::SwapBuffer() const
{
    SwapChain->Present(1, 0); // SyncInterval: VSync 활성화 여부
}

void URenderer::PreparePicking()
{
    D3D11_RENDER_TARGET_BLEND_DESC rtbd;
    ZeroMemory(&rtbd, sizeof(rtbd));
    rtbd.RenderTargetWriteMask = 0; // 모든 색상 채널 쓰기 비활성화

    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0] = rtbd;

    Device->CreateBlendState(&blendDesc, &BlendState);
    
    DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
}

void URenderer::PrepareMain()
{
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

/** 렌더링 파이프라인을 준비 합니다. */
void URenderer::Prepare() const
{
    // 화면 지우기
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    // InputAssembler의 Vertex 해석 방식을 설정
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Rasterization할 Viewport를 설정 
    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);

    /**
* OutputMerger 설정
* 렌더링 파이프라인의 최종 단계로써, 어디에 그릴지(렌더 타겟)와 어떻게 그릴지(블렌딩)를 지정
*/
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);

}

void URenderer::PrepareLine()
{
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}


void URenderer::PreparePickingShader() const
{
    DeviceContext->PSSetShader(UIDPixelShader, nullptr, 0);

    if (ConstantUUIDBuffer)
    {
        DeviceContext->PSSetConstantBuffers(1, 1, &ConstantUUIDBuffer);
    }
}

void URenderer::PrepareMainShader() const
{
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
}

/** 셰이더를 준비 합니다. */
void URenderer::PrepareShader() const
{
    // 기본 셰이더랑 InputLayout을 설정
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);
    
    if (ConstantWorldBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &ConstantWorldBuffer);
    }
}

/**
 * Buffer에 있는 Vertex를 그립니다.
 * @param pBuffer 렌더링에 사용할 버텍스 버퍼에 대한 포인터
 * @param numVertices 버텍스 버퍼에 저장된 버텍스의 총 개수
 */
void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const
{
    UINT Offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &Offset);

    DeviceContext->Draw(numVertices, 0);
}

/**
 * 정점 데이터로 Vertex Buffer를 생성합니다.
 * @param Vertices 버퍼로 변환할 정점 데이터 배열의 포인터
 * @param ByteWidth 버퍼의 총 크기 (바이트 단위)
 * @return 생성된 버텍스 버퍼에 대한 ID3D11Buffer 포인터, 실패 시 nullptr
 *
 * @note 이 함수는 D3D11_USAGE_IMMUTABLE 사용법으로 버퍼를 생성합니다.
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(const FVertexSimple* Vertices, UINT ByteWidth) const
{
    D3D11_BUFFER_DESC VertexBufferDesc = {};
    VertexBufferDesc.ByteWidth = ByteWidth;
    VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA VertexBufferSRD = {};
    VertexBufferSRD.pSysMem = Vertices;

    ID3D11Buffer* VertexBuffer;
    const HRESULT Result = Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &VertexBuffer);
    if (FAILED(Result))
    {
        return nullptr;
    }
    return VertexBuffer;
}

/** Buffer를 해제합니다. */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* pBuffer) const
{
    pBuffer->Release();
}

void URenderer::UpdateConstantView(UObject Target, UCamera Camera) const
{
    if (!ConstantWorldBuffer) return;

    D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

    DirectX::XMMATRIX ScaleMatrix = DirectX::XMMatrixScalingFromVector(CastVecToXMV(FVector(Target.Radius,Target.Radius,Target.Radius)));
    DirectX::XMMATRIX RotationMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(CastVecToXMV(Target.Rotation));
    DirectX::XMMATRIX TranslationMatrix = DirectX::XMMatrixTranslationFromVector(CastVecToXMV(Target.Location));

    DirectX::XMMATRIX WorldMatrix = TranslationMatrix * RotationMatrix * ScaleMatrix;

    FVector CamForwardVec = Camera.Location + Camera.GetForward();
    
    DirectX::XMMATRIX ViewMatrix = DirectX::XMMatrixLookAtLH(CastVecToXMV(Camera.Location), CastVecToXMV(CamForwardVec), CastVecToXMV(Camera.UpVector));

    DirectX::XMMATRIX ProjMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1, 0.1f, 100.0f);

    
    DeviceContext->Map(ConstantWorldBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
    {
        FMatrixConstants* Constants = static_cast<FMatrixConstants*>(ConstantBufferMSR.pData);
        Constants->World = DirectX::XMMatrixTranspose(WorldMatrix);
        Constants->View = DirectX::XMMatrixTranspose(ViewMatrix);
        Constants->Proj = DirectX::XMMatrixTranspose(ProjMatrix);
    }
    DeviceContext->Unmap(ConstantWorldBuffer, 0);
}

void URenderer::UpdateConstantUUID(DirectX::XMFLOAT4 UUIDColor) const
{
    if (!ConstantUUIDBuffer) return;

    D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

    UUIDColor = DirectX::XMFLOAT4(UUIDColor.x/255.0f, UUIDColor.y/255.0f, UUIDColor.z/255.0f, UUIDColor.w/255.0f);
    
    DeviceContext->Map(ConstantUUIDBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
    {
        FUUIDConstants* Constants = static_cast<FUUIDConstants*>(ConstantBufferMSR.pData);
        Constants->UUIDColor = UUIDColor;
    }
    DeviceContext->Unmap(ConstantUUIDBuffer, 0);
}

/** Direct3D Device 및 SwapChain을 생성합니다. */
void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // SwapChain 구조체 초기화
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferDesc.Width = 0;                            // 창 크기에 맞게 자동으로 설정
    SwapChainDesc.BufferDesc.Height = 0;                           // 창 크기에 맞게 자동으로 설정
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 색상 포멧
    SwapChainDesc.SampleDesc.Count = 1;                            // 멀티 샘플링 비활성화
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // 렌더 타겟으로 설정
    SwapChainDesc.BufferCount = 2;                                 // 더블 버퍼링
    SwapChainDesc.OutputWindow = hWindow;                          // 렌더링할 창 핸들
    SwapChainDesc.Windowed = TRUE;                                 // 창 모드
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;      // 스왑 방식

    // Direct3D Device와 SwapChain을 생성
    D3D11CreateDeviceAndSwapChain(
        // 입력 매개변수
        nullptr,                                                       // 디바이스를 만들 때 사용할 비디오 어댑터에 대한 포인터
        D3D_DRIVER_TYPE_HARDWARE,                                      // 만들 드라이버 유형을 나타내는 D3D_DRIVER_TYPE 열거형 값
        nullptr,                                                       // 소프트웨어 래스터라이저를 구현하는 DLL에 대한 핸들
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,  // 사용할 런타임 계층을 지정하는 D3D11_CREATE_DEVICE_FLAG 열거형 값들의 조합
        FeatureLevels,                                                 // 만들려는 기능 수준의 순서를 결정하는 D3D_FEATURE_LEVEL 배열에 대한 포인터
        ARRAYSIZE(FeatureLevels),                                      // pFeatureLevels 배열의 요소 수
        D3D11_SDK_VERSION,                                             // SDK 버전. 주로 D3D11_SDK_VERSION을 사용
        &SwapChainDesc,                                                // SwapChain 설정과 관련된 DXGI_SWAP_CHAIN_DESC 구조체에 대한 포인터

        // 출력 매개변수
        &SwapChain,                                                    // 생성된 IDXGISwapChain 인터페이스에 대한 포인터
        &Device,                                                       // 생성된 ID3D11Device 인터페이스에 대한 포인터
        nullptr,                                                       // 선택된 기능 수준을 나타내는 D3D_FEATURE_LEVEL 값을 반환
        &DeviceContext                                                 // 생성된 ID3D11DeviceContext 인터페이스에 대한 포인터
    );

    // 생성된 SwapChain의 정보 가져오기
    SwapChain->GetDesc(&SwapChainDesc);

    // 뷰포트 정보 설정
    ViewportInfo = {
        0.0f, 0.0f,
        static_cast<float>(SwapChainDesc.BufferDesc.Width), static_cast<float>(SwapChainDesc.BufferDesc.Height),
        0.0f, 1.0f
    };
    //Windown Width, Height 구하기
    RECT Rect;
    int Width , Height;
    if (GetClientRect(hWindow , &Rect)) {
        Width = Rect.right - Rect.left;
        Height = Rect.bottom - Rect.top;
    }
    ViewportInfo.Width = Width;
    ViewportInfo.Height = Height; 
    
}

/** Direct3D Device 및 SwapChain을 해제합니다.  */
void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남이있는 GPU 명령 실행
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

/** 프레임 버퍼를 생성합니다. */
void URenderer::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer));

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC FrameBufferRTVDesc = {};
    FrameBufferRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;      // 색상 포맷
    FrameBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &FrameBufferRTVDesc, &FrameBufferRTV);
}

/** 프레임 버퍼를 해제합니다. */
void URenderer::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }
}

/** 레스터라이즈 상태를 생성합니다. */
void URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC RasterizerDesc = {};
    RasterizerDesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    RasterizerDesc.CullMode = D3D11_CULL_BACK;  // 백 페이스 컬링

    Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState);
}

/** 레스터라이저 상태를 해제합니다. */
void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }
}

// void URenderer::CreateStagingTexture()
// {
//     D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
//     ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
//
//     depthStencilDesc.DepthEnable = TRUE;
//     depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
//     depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
//
//     ID3D11DepthStencilState* depthStencilState;
//     Device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
// }
//
// void URenderer::ReleaseStagingTexture()
// {
//     if (StagingTexture)
//     {
//         StagingTexture->Release();
//         StagingTexture = nullptr;
//     }
//     if (BackBuffer)
//     {
//         BackBuffer->Release();
//         BackBuffer = nullptr;
//     }
// }

DirectX::XMFLOAT4 URenderer::GetPixel(FVector MPos)
{
    // 백 버퍼 가져오기
    ID3D11Texture2D* backBuffer;
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

    // 스테이징 텍스처 생성 (CPU 읽기 가능)
    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture;
    Device->CreateTexture2D(&desc, nullptr, &stagingTexture);

    // 백 버퍼 → 스테이징 텍스처 복사
    DeviceContext->CopySubresourceRegion()
    DeviceContext->CopyResource(stagingTexture, backBuffer);

    ID3D11Texture2D* ResolveTexture;
    
    desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    Device->CreateTexture2D(&desc, nullptr, &ResolveTexture);
    
    DeviceContext->ResolveSubresource(ResolveTexture, 0, stagingTexture, 0, DXGI_FORMAT_R32G32B32_FLOAT);

    D3D11_MAPPED_SUBRESOURCE mapped;
    DeviceContext->Map(ResolveTexture, 0, D3D11_MAP_READ, 0, &mapped);
    
    // 픽셀 좌표 계산 (클릭 위치 기반)
    UINT x = MPos.X;
    UINT y = MPos.Y;
    UINT pixelIndex = y * mapped.RowPitch / 128 + x; // 4 bytes per pixel (RGBA)

    BYTE* pixels = (BYTE*)mapped.pData;
    // RGBA 값 추출
    DirectX::XMFLOAT4 color = {
        static_cast<float>(pixels[pixelIndex * 4 + 0]),
        static_cast<float>(pixels[pixelIndex * 4 + 1]),
        static_cast<float>(pixels[pixelIndex * 4 + 2]),
        static_cast<float>(pixels[pixelIndex * 4 + 3]),
    };
    // 매핑 해제
    DeviceContext->Unmap(stagingTexture, 0);
    
    stagingTexture->Release();
    backBuffer->Release();
    
    return color;
}

void URenderer::CreateDepthStencilBuffer(FVector WindowSize)
{
    //텍스쳐 생성
    D3D11_TEXTURE2D_DESC DepthDesc = {};
    DepthDesc.Width = WindowSize.X;
    DepthDesc.Height = WindowSize.Y;
    DepthDesc.MipLevels = 1;
    DepthDesc.ArraySize = 1;
    DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthDesc.SampleDesc.Count = 1;
    DepthDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* DepthStencilTexture;
    Device->CreateTexture2D(&DepthDesc, nullptr, &DepthStencilTexture);

    //뷰 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
    DepthStencilViewDesc.Format = DepthDesc.Format;
    DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    Device->CreateDepthStencilView(DepthStencilTexture, &DepthStencilViewDesc, &DepthStencilView);
    
    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
    DepthStencilDesc.DepthEnable = TRUE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    ID3D11DepthStencilState* DepthStencilState;
    Device->CreateDepthStencilState(&DepthStencilDesc, &DepthStencilState);

    DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
}

void URenderer::ReleaseDepthStencilBuffer()
{
    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }
}