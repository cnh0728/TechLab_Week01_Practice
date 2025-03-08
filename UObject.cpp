#include <cstdlib>

#include "UObject.h"
#include "URenderer.h"

UObject::UObject(): Location{
    static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
    static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
	static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f
	// 0.0f
}
, Velocity{
    static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
    static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
    static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f
	// 0.0f
}
, Radius{ static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.15f + 0.05f }
, Mass { (4.0f / 3.0f) * 3.141592f * powf(Radius, 3) * 1000.0f }
, Scale{
	static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
	static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f,
	static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 2.0f) - 1.0f
},UUID{
	UUID_GEN++
}
{
}

bool UObject::CheckCollision(const UObject& A, const UObject& B)
{
	const float Distance = (A.Location - B.Location).Length();
	return Distance <= (A.Radius + B.Radius);
}

void UObject::Update(float DeltaTime)
{
	if (!bApplyGravity)
	{
		Location += Velocity * DeltaTime;
	}

	// Add Velocity
	if (Location.X - Radius < -1.0f)
	{
		Location.X = -1.0f + Radius;
		HandleWallCollision(FVector(1, 0, 0));
	}
	else if (Location.X + Radius > 1.0f)
	{
		Location.X = 1.0f - Radius;
		HandleWallCollision(FVector(-1, 0, 0));
	}

	if (Location.Y - Radius < -1.0f)
	{
		Location.Y = -1.0f + Radius;
		HandleWallCollision(FVector(0, 1, 0));
	}
	else if (Location.Y + Radius > 1.0f)
	{
		Location.Y = 1.0f - Radius;
		HandleWallCollision(FVector(0, -1, 0));
	}

	if (Location.Z - Radius < -1.0f)
	{
		Location.Z = -1.0f + Radius;
		HandleWallCollision(FVector(0, 0, 1));
	}
	else if (Location.Z + Radius > 1.0f)
	{
		Location.Z = 1.0f - Radius;
		HandleWallCollision(FVector(0, 0, -1));
	}

}

void UObject::FixedUpdate(float FixedTime)
{
	if (!bApplyGravity) return;

	Location += Velocity * FixedTime;
	Velocity.Y += Gravity * FixedTime;
}

void UObject::UpdateConstant(const URenderer& Renderer) const
{
	Renderer.UpdateConstant(Location, Radius);
}

void UObject::UpdateConstantView(const URenderer& Renderer, const UCamera& Camera) const
{
	Renderer.UpdateConstantView(*this, Camera);
}

void UObject::HandleWallCollision(const FVector& WallNormal)
{
	// 속도를 벽면에 수직인 성분과 평행한 성분으로 분해
	FVector VelocityNormal = WallNormal * FVector::DotProduct(Velocity, WallNormal);
	const FVector VelocityTangent = Velocity - VelocityNormal;

	// 수직 속도 성분에 반발 계수를 적용하여 반사
	VelocityNormal = -VelocityNormal * BounceFactor;

	// 반사된 수직 속도와 마찰이 적용된 평행 속도를 합하여 최종 속도 계산
	Velocity = VelocityNormal + VelocityTangent * (1.0f - Friction);
}

void UObject::HandleBallCollision(UObject& OtherBall)
{
	// 충돌 법선 벡터와 상대속도 계산
	const FVector Normal = (OtherBall.Location - Location).Normalize();
	const FVector RelativeVelocity = OtherBall.Velocity - Velocity;

	const float VelocityAlongNormal = FVector::DotProduct(RelativeVelocity, Normal);

	// 이미 서로 멀어지고 있는 경우 무시
	if (VelocityAlongNormal > 0) return;

	// 충격량 계산
	const float e = min(BounceFactor, OtherBall.BounceFactor);  // 반발 계수를 둘중 더 작은걸로 설정
	float j = -(1 + e) * VelocityAlongNormal;
	j /= 1 / Mass + 1 / OtherBall.Mass;

	// 속도 업데이트
	const FVector Impulse = Normal * j;
	Velocity -= Impulse / Mass;
	OtherBall.Velocity += Impulse / OtherBall.Mass;

	// 마찰 적용
	FVector Tangent = RelativeVelocity - Normal * VelocityAlongNormal;
	if (Tangent.LengthSquared() > 0.0001f)  // 탄젠트의 길이가 매우 작으면 건너뛰기
	{
		Tangent = Tangent.Normalize();

		// 탄젠트 충격량 계산
		float JT = -FVector::DotProduct(RelativeVelocity, Tangent);  // 접선 방향 상대 속도에 기반한 충격량 크기
		JT /= 1 / Mass + 1 / OtherBall.Mass;                               // 두 물체의 유효 질량

		const float MuT = min(Friction, OtherBall.Friction);
		FVector FrictionImpulse;
		if (fabsf(JT) < j * MuT)
		{
			// 실제 마찰력 사용
			FrictionImpulse = Tangent * JT;
		}
		else
		{
			// 한계치를 초과시 j * MuT로 제한
			FrictionImpulse = Tangent * -j * MuT;
		}

		// 마찰력 적용
		Velocity -= FrictionImpulse / Mass;
		OtherBall.Velocity += FrictionImpulse / OtherBall.Mass;
	}

	// 겹침 해결
	const float Penetration = Radius + OtherBall.Radius - (OtherBall.Location - Location).Length();
	const FVector Correction = Normal * Penetration / (Mass + OtherBall.Mass) * 0.8f;
	Location -= Correction * Mass;
	OtherBall.Location += Correction * OtherBall.Mass;
}