#include "InputSystem.h"

#include "Enum.h"
#include "UCamera.h"

InputSystem::InputSystem()
{
    for (bool& key : _keys)
    {
        key = false;
    }

    for (bool& m : mouse)
    {
        m=false;
    }
}

bool InputSystem::IsPressedKey(EKeyCode key) const
{
    return _keys[static_cast<uint8_t>(key)];
}

void InputSystem::KeyDown(EKeyCode key)
{
    _keys[static_cast<uint8_t>(key)] = true;
}

void InputSystem::KeyUp(EKeyCode key)
{
    _keys[static_cast<uint8_t>(key)] = false;
}

std::vector<EKeyCode> InputSystem::GetPressedKeys() {
    std::vector<EKeyCode> ret;

    for (int i = 0; i < 256; i++) {
        if (_keys[ i ]) {
            ret.push_back(static_cast< EKeyCode >( i ));
        }
    }

    return ret;
}
void InputSystem::MouseKeyDown(FVector MouseDownPoint, FVector WindowSize, int isRight) {
    mouse[isRight] = true;
    onceMouse[isRight] = true;
    MouseKeyDownPos[isRight] = MouseDownPoint;
    MouseKeyDownNDCPos[isRight] = CalNDCPos(MouseKeyDownPos[isRight], WindowSize);
} //MouseKeyDownPos 설정

void InputSystem::MouseKeyUp(FVector MouseUpPoint, FVector WindowSize, int isRight) {
    mouse[isRight] = false;
    onceMouse[isRight] = false;
}

void InputSystem::ExpireOnceMouse()
{
    for (bool& i : onceMouse)
    {
        i = false;
    }  
}

FVector InputSystem::CalNDCPos(FVector MousePos, FVector WindowSize)
{
    return {( MousePos.X / ( WindowSize.X / 2 ) ) - 1, ( MousePos.Y / ( WindowSize.Y / 2 ) ) - 1, 0};
}

#include <unordered_map>
#include <utility>
#include <cmath>

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (std::pair<T1, T2> const& v) const {
        auto h1 = std::hash<T1>{}( v.first );
        auto h2 = std::hash<T2>{}( v.second );
        return h1 ^ h2;
    }
};

std::unordered_map<Direction, FVector> DicKeyAddVector //speed는 곱해서 쓰자
{
    {Left, FVector(-10, 0, 0)},
    {Right, FVector(10, 0, 0)},
    {Top, FVector(0, 0, 10)},
    {Bottom, FVector(0, 0, -10)},
};

InputHandler::InputHandler() {

}
#include <iostream>

void InputHandler::HandleCameraInput(UCamera* Camera) {

    FVector NewVelocity(0, 0, 0);

    if (InputSystem::Get().IsPressedMouse(true) == false)
    {
        Camera->SetVelocity(NewVelocity);
        return;
    }
    //전프레임이랑 비교
    //x좌표 받아와서 x만큼 x축회전
    //y좌표 받아와서 y만큼 y축 회전
    FVector MousePrePos = InputSystem::Get().GetMousePrePos();
    FVector MousePos = InputSystem::Get().GetMousePos();
    FVector DeltaPos = MousePos - MousePrePos;
    Camera->Rotation = FVector(Camera->Rotation.X, Camera->Rotation.Y - DeltaPos.Y, Camera->Rotation.Z - DeltaPos.X);

    if (InputSystem::Get().IsPressedKey(EKeyCode::A)) {
        NewVelocity += Camera->GetRight();
    }
    if (InputSystem::Get().IsPressedKey(EKeyCode::D)) {
        NewVelocity -= Camera->GetRight();
    }
    if (InputSystem::Get().IsPressedKey(EKeyCode::W)) {
        NewVelocity += Camera->GetForward();
    }
    if (InputSystem::Get().IsPressedKey(EKeyCode::S)) {
        NewVelocity -= Camera->GetForward();
    }
    if (InputSystem::Get().IsPressedKey(EKeyCode::Q))
    {
        NewVelocity -= Camera->GetUp();
    }
    if (InputSystem::Get().IsPressedKey(EKeyCode::E))
    {
        NewVelocity += Camera->GetUp();
    }
    if (NewVelocity.Length() > 0.001f)
    {
        NewVelocity = NewVelocity.Normalize();
    }
    
    //회전이랑 마우스클릭 구현 카메라로 해야할듯?
    
    Camera->SetVelocity(NewVelocity);
}

void InputHandler::InputUpdate(UCamera* Camera) {
    
    HandleCameraInput(Camera);
    
}