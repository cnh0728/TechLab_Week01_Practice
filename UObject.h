#pragma once

#include <cstdint>

#include "Enum.h"
#include "Core/Math/Vector.h"

namespace DirectX
{
	struct XMFLOAT4;
}

class UObject
{
public:
	static int UUID_GEN;
	
	int UUID;
	
	FVector Location;
	FVector Rotation;
	FVector Scale;
	
	FVector Velocity;
	float Radius;
	float Mass;

	float Friction = 0.01f;      // 마찰 계수
	float BounceFactor = 0.85f;  // 반발 계수

	bool   bApplyGravity = false;
	static float Gravity;

	
public:
	UObject();

	static bool CheckCollision(const UObject& A, const UObject& B);

	void Update(float DeltaTime);

	void FixedUpdate(float FixedTime);
	
	void UpdateConstantView(const class URenderer& Renderer, const class UCamera& Camera) const;

	void UpdateConstantUUID(const URenderer& Renderer, DirectX::XMFLOAT4 UUIDColor) const;
	void HandleWallCollision(const FVector& WallNormal);

	void HandleBallCollision(UObject& OtherBall);
};
