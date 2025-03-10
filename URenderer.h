#pragma once
#include "Enum.h"
#include "Source/Core/Math/Vector.h"

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "UCamera.h"
#include "UObject.h"

class URenderer
{
private:
    struct alignas(16) FUUIDConstants
    {
        DirectX::XMFLOAT4 UUIDColor;
    };

    struct alignas(16) FMatrixConstants
    {
        DirectX::XMMATRIX World;
        DirectX::XMMATRIX View;
        DirectX::XMMATRIX Proj;  
    };

public:
    /** Renderer를 초기화 합니다. */
    void Create(HWND hWindow);

    void CreatePickingTexture(HWND hWnd);
    
    /** Renderer에 사용된 모든 리소스를 해제합니다. */
    void Release();

    void CreateShader();
    void ReleaseShader();

    void CreateDepthStencilBuffer(FVector WindowSize);
    void ReleaseDepthStencilBuffer();
    
	void CreateConstantBuffer();
	void ReleaseConstantBuffer();

    void CreatePickingShader();
    void PreparePicking();
    void PrepareMain();
    void PreparePickingShader() const;
    void PrepareMainShader() const;

    /** 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력 */
    void SwapBuffer() const;

    /** 렌더링 파이프라인을 준비 합니다. */
    void Prepare() const;

    /** 셰이더를 준비 합니다. */
    void PrepareShader() const;

    /**
     * Buffer에 있는 Vertex를 그립니다.
     * @param pBuffer 렌더링에 사용할 버텍스 버퍼에 대한 포인터
     * @param numVertices 버텍스 버퍼에 저장된 버텍스의 총 개수
     */
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;
    void RenderPickingTexture();

    /**
     * 정점 데이터로 Vertex Buffer를 생성합니다.
     * @param Vertices 버퍼로 변환할 정점 데이터 배열의 포인터
     * @param ByteWidth 버퍼의 총 크기 (바이트 단위)
     * @return 생성된 버텍스 버퍼에 대한 ID3D11Buffer 포인터, 실패 시 nullptr
     *
     * @note 이 함수는 D3D11_USAGE_IMMUTABLE 사용법으로 버퍼를 생성합니다.
     */
    ID3D11Buffer* CreateVertexBuffer(const FVertexSimple* Vertices, UINT ByteWidth) const;

    /** Buffer를 해제합니다. */
    void ReleaseVertexBuffer(ID3D11Buffer* pBuffer) const;
    
    void UpdateConstantView(UObject OriginTargetPos, UCamera Camera) const;
    void UpdateConstantUUID(DirectX::XMFLOAT4 UUIDColor) const;
    void UpdateConstantPick(DirectX::XMFLOAT4 UUIDColor) const;
    DirectX::XMFLOAT4 GetPixel(FVector MPos);

    ID3D11Device* GetDevice() const { return Device; }
    ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
    void PrepareLine();

    void ReleaseRasterizerState();

protected:
    /** Direct3D Device 및 SwapChain을 생성합니다. */
    void CreateDeviceAndSwapChain(HWND hWindow);

    /** Direct3D Device 및 SwapChain을 해제합니다.  */
    void ReleaseDeviceAndSwapChain();

    /** 프레임 버퍼를 생성합니다. */
    void CreateFrameBuffer();

    /** 프레임 버퍼를 해제합니다. */
    void ReleaseFrameBuffer();

    /** 레스터라이즈 상태를 생성합니다. */
    void CreateRasterizerState();

    /** 레스터라이저 상태를 해제합니다. */

    DirectX::XMVECTOR CastVecToXMV(FVector Vec) const{ return DirectX::XMVectorSet(Vec.X, Vec.Y, Vec.Z, 1.0f); }
    
protected:
    // Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
    ID3D11Device* Device = nullptr;                         // GPU와 통신하기 위한 Direct3D 장치
    ID3D11DeviceContext* DeviceContext = nullptr;           // GPU 명령 실행을 담당하는 컨텍스트
    IDXGISwapChain* SwapChain = nullptr;                    // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

    // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
    ID3D11Texture2D* FrameBuffer = nullptr;                 // 화면 출력용 텍스처
    ID3D11Texture2D* PickingFrameBuffer = nullptr;                 // 화면 출력용 텍스처
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;       // 텍스처를 렌더 타겟으로 사용하는 뷰
    ID3D11RenderTargetView* PickingFrameBufferRTV = nullptr;       // 텍스처를 렌더 타겟으로 사용하는 뷰
    ID3D11RasterizerState* RasterizerState = nullptr;       // 래스터라이저 상태(컬링, 채우기 모드 등 정의)
    ID3D11Buffer* ConstantWorldBuffer = nullptr;                 // 뷰 상수 버퍼
    ID3D11Buffer* ConstantUUIDBuffer = nullptr;                 // 뷰 상수 버퍼

    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11BlendState* BlendState = nullptr;

    FLOAT PickingClearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo = {};                       // 렌더링 영역을 정의하는 뷰포트 정보

    
    // Shader를 렌더링할 때 사용되는 변수들
    ID3D11VertexShader* SimpleVertexShader = nullptr;       // Vertex 데이터를 처리하는 Vertex 셰이더
    ID3D11PixelShader* SimplePixelShader = nullptr;         // Pixel의 색상을 결정하는 Pixel 셰이더
    ID3D11PixelShader* UIDPixelShader = nullptr;         // Pixel의 색상을 결정하는 Pixel 셰이더
    ID3D11InputLayout* SimpleInputLayout = nullptr;         // Vertex 셰이더 입력 레이아웃 정의

    UINT msaaQuality = 0;
    unsigned int Stride = 0;
};
