#pragma once

#include <cstdint>

#include "Core/Math/Vector.h"

class UObject
{
public:
	static uint8_t UUID_GEN;
	
	uint8_t UUID;
	
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

	void UpdateConstant(const class URenderer& Renderer) const;

	void UpdateConstantView(const URenderer& Renderer, const class UCamera& Camera) const;
	
	void HandleWallCollision(const FVector& WallNormal);

	void HandleBallCollision(UObject& OtherBall);
};