#include "UCamera.h"
#include <cmath>

#define TORAD 3.14159265358979323846/180

FVector UCamera::GetForward()
{
    return FVector(
            std::cos(Rotation.Z * TORAD) * std::cos(Rotation.Y * TORAD),
            std::sin(Rotation.Y * TORAD),
            std::sin(Rotation.Z * TORAD) * std::cos(Rotation.Y * TORAD)
        ).Normalize();
}

FVector UCamera::GetRight()
{
    return FVector::CrossProduct(GetForward(), FVector(0, 1, 0));
}

FVector UCamera::GetUp()
{
    return FVector::CrossProduct(GetRight(), GetForward());
}

void UCamera::FixedUpdate(float DeltaTime)
{
    Location += Velocity * DeltaTime;
}


