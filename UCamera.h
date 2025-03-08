#pragma once

#include "Core/Math/Vector.h"

class UCamera
{
public:
    FVector Location = FVector(-5, 0, 1);
    FVector UpVector = FVector(0, 1, 0); //업벡터를 조금씩 틀어주면 rotation할듯
    FVector Rotation = FVector(0, 0, 0);

    void SetVelocity(FVector NewVelocity) { Velocity = NewVelocity; }
    void SetUpVector(FVector NewUpVector) { UpVector = NewUpVector; }

    FVector GetForward();
    FVector GetRight();
    FVector GetUp();

    void FixedUpdate(float DeltaTime);
    
private:
    FVector Velocity;
    
};