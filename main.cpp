#include <iostream>
#include <memory>
#include <Windows.h>

#include "ImGui/imgui.h"
#include "Imgui/imgui_internal.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#include "Enum.h"
#include "InputSystem.h"
#include "URenderer.h"
#include "PrimitiveVertices.h"
#include "UObject.h"

DirectX::XMFLOAT4 UUIDToFLOAT4(int UUID)
{
	float a = (UUID >> 24) & 0xff;
	float b = (UUID >> 16) & 0xff;
	float g = (UUID >> 8) & 0xff;
	float r = UUID & 0xff;
	
	DirectX::XMFLOAT4 color = {r, g, b, a};
    
	return color;
}

int FLOAT4ToUUID(DirectX::XMFLOAT4 f)
{
	return (static_cast<int>(f.w)<<24) | (static_cast<int>(f.z)<<16) | (static_cast<int>(f.y)<<8) | (static_cast<int>(f.x));
}

FVector GetWndWH(HWND hWnd)
{
	RECT Rect;
	int Width , Height;
	if (GetClientRect(hWnd , &Rect)) {
		Width = Rect.right - Rect.left;
		Height = Rect.bottom - Rect.top;
	}

	return FVector(Width, Height, 0.0f);
}

void HandleMouseEvent(HWND hWnd, LPARAM lParam, bool isDown, bool isRight)
{
	POINTS Pts = MAKEPOINTS(lParam);
	FVector WH = GetWndWH(hWnd);
	if (isDown)
	{
		InputSystem::Get().MouseKeyDown(FVector(Pts.x , Pts.y , 0), FVector(WH.X , WH.Y , 0), isRight);
	}else
	{
		InputSystem::Get().MouseKeyUp(FVector(Pts.x , Pts.y , 0), FVector(WH.X , WH.Y , 0), isRight);
	}
}

void HandleMouseMove()
{
	POINT Pts;
	if (GetCursorPos(&Pts))
	{
		FVector PtV = FVector(Pts.x , Pts.y, 0);
		InputSystem::Get().SetMousePos(PtV);
	}
}

// ImGui WndProc 정의
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 각종 윈도우 관련 메시지(이벤트)를 처리하는 함수
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // ImGui의 메시지를 처리
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    switch (uMsg)
    {
    // 창이 제거될 때 (창 닫기, Alt+F4 등)
    case WM_DESTROY:
        PostQuitMessage(0); // 프로그램 종료
        break;
    case WM_KEYDOWN:
    	InputSystem::Get().KeyDown(static_cast<EKeyCode>(wParam));
    	break;
    case WM_KEYUP:
    	InputSystem::Get().KeyUp(static_cast<EKeyCode>( wParam ));
    	break;
    case WM_LBUTTONDOWN:
    	HandleMouseEvent(hWnd, lParam, true, false);
    	break;
    case WM_LBUTTONUP:
    	HandleMouseEvent(hWnd, lParam, false, false);
    	break;
    case WM_RBUTTONDOWN:
    	HandleMouseEvent(hWnd, lParam, true, true);
    	break;
	case WM_RBUTTONUP:
		HandleMouseEvent(hWnd, lParam, false, true);
    	break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

float UObject::Gravity = 9.81f;
unsigned int UObject::UUID_GEN = 0;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
#pragma region Init Window
    // 사용 안하는 파라미터들
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    // 윈도우 클래스 이름 및 타이틀 이름
    constexpr WCHAR WndClassName[] = L"DX11 Test Window Class";
    constexpr WCHAR WndTitle[] = L"DX11 Test Window";

    // 윈도우 클래스 등록
    WNDCLASS WndClass = {0, WndProc, 0, 0, hInstance, nullptr, nullptr, nullptr, nullptr, WndClassName};
    RegisterClass(&WndClass);

    // 1024 x 1024로 윈도우 생성
    const HWND hWnd = CreateWindowEx(
        0, WndClassName, WndTitle,
        WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 1024,
        nullptr, nullptr, hInstance, nullptr
    );
#pragma endregion Init Window
	AllocConsole(); // 콘솔 창 생성

	// 표준 출력 및 입력을 콘솔과 연결
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

	// 콘솔 창 위치 조정
	HWND consoleWindow = GetConsoleWindow();
	SetWindowPos(consoleWindow, 0, 1100, 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	
	std::cout << "Debug Console Opened!" << '\n';
#pragma region Init Renderer & ImGui
    // 렌더러 초기화
    URenderer Renderer;
    Renderer.Create(hWnd);
    Renderer.CreateShader();
    Renderer.CreateConstantBuffer();
	Renderer.CreateDepthStencilBuffer(GetWndWH(hWnd));
	// ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // ImGui Backend 초기화
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());
#pragma endregion Init Renderer & ImGui

#pragma region Create Vertex Buffer
	ID3D11Buffer* VertexBufferSphere = Renderer.CreateVertexBuffer(SphereVertices, sizeof(SphereVertices));

	ID3D11Buffer* VertexBufferAxisX = Renderer.CreateVertexBuffer(AxisXVertices, sizeof(AxisXVertices));
	ID3D11Buffer* VertexBufferAxisY = Renderer.CreateVertexBuffer(AxisYVertices, sizeof(AxisYVertices));
	ID3D11Buffer* VertexBufferAxisZ = Renderer.CreateVertexBuffer(AxisZVertices, sizeof(AxisZVertices));
#pragma endregion Create Vertex Buffer

    // FPS 제한
    constexpr int TargetFPS = 60;
    constexpr double TargetDeltaTime = 1000.0f / TargetFPS; // 1 FPS의 목표 시간 (ms)

    // 고성능 타이머 초기화
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER StartTime;
    QueryPerformanceCounter(&StartTime);

    float Accumulator = 0.0; // Fixed Update에 사용되는 값
    constexpr float FixedTimeStep = 1.0f / TargetFPS;
	
	// UBalls 배열
	int ArrSize = 1;
	int ArrCap = 4;
	UObject** Balls = new UObject*[ArrCap];
	Balls[0] = new UObject;

	int NumOfBalls = 1;

	std::unique_ptr<UObject> zeroObject = std::make_unique<UObject>();
	zeroObject->Location = FVector(0, 0, 0);
	zeroObject->Scale = FVector(1, 1, 1);
	zeroObject->Rotation = FVector(0, 0, 0);
	
	std::unique_ptr<UCamera> Camera = std::make_unique<UCamera>();
	// Camera->SetCameraPosition(FVector(0, 0, -5));
	std::unique_ptr<InputHandler> Input = std::make_unique<InputHandler>();
	
	// Main Loop
    bool bIsExit = false;
    while (bIsExit == false)
    {
        // DeltaTime 계산 (초 단위)
        const LARGE_INTEGER EndTime = StartTime;
        QueryPerformanceCounter(&StartTime);

        const float DeltaTime =
            static_cast<float>(StartTime.QuadPart - EndTime.QuadPart) / static_cast<float>(Frequency.QuadPart);
		
        // 누적 시간 추가
        Accumulator += DeltaTime;

    	InputSystem::Get().ExpireOnceMouse();
    	
        // 메시지(이벤트) 처리
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            // 키 입력 메시지를 번역
            TranslateMessage(&msg);

            // 메시지를 등록한 Proc에 전달
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }
    	
		// Update 로직
		Input->InputUpdate(Camera.get());
		HandleMouseMove();
    	
		for (int i = 0; i < ArrSize; ++i)
		{
			Balls[i]->Update(DeltaTime);
		}
    	
    	// FixedTimeStep 만큼 업데이트
    	while (Accumulator >= FixedTimeStep)
    	{
			Camera->FixedUpdate(DeltaTime);
    		
    		for (int i = 0; i < ArrSize; ++i)
    		{
				Balls[i]->FixedUpdate(FixedTimeStep);
    		}

    		// 공 충돌 처리
    		for (int i = 0; i < ArrSize; ++i)
    		{
    			for (int j = i + 1; j < ArrSize; ++j)
    			{
    				if (UObject::CheckCollision(*Balls[i], *Balls[j]))
    				{
    					Balls[i]->HandleBallCollision(*Balls[j]);
    				}
    			}
    		}

    		Accumulator -= FixedTimeStep;
    	}

        // 렌더링 준비 작업
    	//기본적으로 해줘야하는거
        Renderer.Prepare();
		Renderer.PrepareShader();

    	//이거 마우스 다운일때만 해도되지않나

    	if (InputSystem::Get().GetMouseDown(false))
    	{
    		Renderer.PreparePicking();
    		Renderer.PreparePickingShader();

    		for (int i = 0; i < ArrSize; ++i)
    		{
    			Balls[i]->UpdateConstantView(Renderer, *Camera);
    			Balls[i]->UpdateConstantUUID(Renderer, UUIDToFLOAT4(Balls[i]->UUID));
    			Renderer.RenderPrimitive(VertexBufferSphere, ARRAYSIZE(SphereVertices));
    		}
    		
    	}

    	Renderer.PrepareMain();
    	Renderer.PrepareMainShader();
    	for (int i = 0; i < ArrSize; ++i)
    	{
    		Balls[i]->UpdateConstantView(Renderer, *Camera );
    		Renderer.RenderPrimitive(VertexBufferSphere, ARRAYSIZE(SphereVertices));
    	}
    	
    	if (InputSystem::Get().GetMouseDown(false))
    	{
    		POINT pt;
    		GetCursorPos(&pt);
    		ScreenToClient(hWnd, &pt);
    		
    		DirectX::XMFLOAT4 color = Renderer.GetPixel(FVector(pt.x, pt.y, 0));
	    
    		std::cout << FLOAT4ToUUID(color) << "\n";
    	}


    	// Renderer.RenderPickingTexture();

    	#pragma region DrawAxis

		zeroObject->UpdateConstantView(Renderer, *Camera);
    	Renderer.PrepareLine();
    	
		Renderer.RenderPrimitive(VertexBufferAxisX, ARRAYSIZE(AxisXVertices));
    	Renderer.RenderPrimitive(VertexBufferAxisY, ARRAYSIZE(AxisYVertices));
    	Renderer.RenderPrimitive(VertexBufferAxisZ, ARRAYSIZE(AxisZVertices));
    	
#pragma endregion DrawAxis

    	// ImGui Frame 생성
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("DX11 Property Window");
        {
            ImGui::Text("Hello, World!");
        	ImGui::Text("FPS: %.3f", ImGui::GetIO().Framerate);
        	ImGui::Text("Size: %d, Capacity: %d", ArrSize, ArrCap);
        	if (ImGui::Checkbox("Gravity", &Balls[0]->bApplyGravity))
        	{
        		for (int i = 1; i < ArrSize; ++i)
        		{
        			Balls[i]->bApplyGravity = Balls[0]->bApplyGravity;
        		}
        	}
        	if (Balls[0]->bApplyGravity)
        	{
        		ImGui::SliderFloat("Gravity Factor", &UObject::Gravity, -20.0f, 20.0f);
        	}

        	if (ImGui::SliderFloat("Bounce Factor", &Balls[0]->BounceFactor, 0.0f, 1.0f))
        	{
        		for (int i = 1; i < ArrSize; ++i)
        		{
        			Balls[i]->BounceFactor = Balls[0]->BounceFactor;
        		}
        	}
        	if (ImGui::SliderFloat("Friction", &Balls[0]->Friction, 0.0f, 1.0f))
        	{
        		for (int i = 1; i < ArrSize; ++i)
        		{
        			Balls[i]->Friction = Balls[0]->Friction;
        		}
        	}

        	ImGui::SliderFloat("CameraX", &Camera->Location.X, -10.0f, 10.0f);
        	ImGui::SliderFloat("CameraY", &Camera->Location.Y, -10.0f, 10.0f);
        	ImGui::SliderFloat("CameraZ", &Camera->Location.Z, -10.0f, 10.0f);

        	ImGui::SliderFloat("CamRotationX", &Camera->Rotation.X, -180.0f, 180.0f);
        	ImGui::SliderFloat("CamRotationY", &Camera->Rotation.Y, -180.0f, 180.0f);
        	ImGui::SliderFloat("CamRotationZ", &Camera->Rotation.Z, -180.0f, 180.0f);
        	
        	if (ImGui::InputInt("Number of Ball", &NumOfBalls))
        	{
        		NumOfBalls = max(NumOfBalls, 1);
        		const int Diff = NumOfBalls - ArrSize;
        		if (Diff > 0)
		        {
			        if (NumOfBalls > ArrCap)
			        {
			        	int NewCap = NumOfBalls > ArrCap * 2
			        		? static_cast<int>(static_cast<float>(NumOfBalls) * 1.5f)
			        		: ArrCap * 2;

			        	UObject** NewBalls = new UObject*[NewCap];
			        	for (int i = 0; i < ArrSize; ++i)
			        	{
			        		NewBalls[i] = Balls[i];
			        	}
			        	delete[] Balls;
			        	ArrCap = NewCap;
			        	Balls = NewBalls;
			        }

			        for (int i = 0; i < Diff; ++i)
			        {
			        	UObject* Ball = new UObject;
			        	Ball->bApplyGravity = Balls[0]->bApplyGravity;
				        Balls[ArrSize + i] = Ball;
			        }
        			ArrSize = NumOfBalls;
		        }
		        else if (Diff < 0)
        		{
		        	for (int i = 0; i < -Diff; ++i)
		        	{
		        		int IndexToRemove = rand() % ArrSize;
		        		delete Balls[IndexToRemove];
		        		Balls[IndexToRemove] = Balls[ArrSize - 1];
		        		ArrSize--;
		        	}
        		}
        	}
        }
        ImGui::End();

        // ImGui 렌더링
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환
        Renderer.SwapBuffer();
        // FPS 제한
        double ElapsedTime;
        do
        {
            Sleep(0);

            LARGE_INTEGER CurrentTime;
            QueryPerformanceCounter(&CurrentTime);

            ElapsedTime = static_cast<double>(CurrentTime.QuadPart - StartTime.QuadPart) * 1000.0 / static_cast<double>(Frequency.QuadPart);
        } while (ElapsedTime < TargetDeltaTime);
    }

	delete[] Balls;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

	Renderer.ReleaseDepthStencilBuffer();
    Renderer.ReleaseConstantBuffer();
    Renderer.ReleaseShader();
    Renderer.Release();
	FreeConsole();
    return 0;
}
