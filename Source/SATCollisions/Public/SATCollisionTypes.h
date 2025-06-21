#pragma once

UENUM(BlueprintType)
enum class ESATCollisionResponse : uint8
{
	Ignore     UMETA(DisplayName = "Ignore"),
	Overlap    UMETA(DisplayName = "Overlap"),
	Block      UMETA(DisplayName = "Block")
};