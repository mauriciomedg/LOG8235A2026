// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StateMachine.generated.h"

class ACharacter;
class APawn;

UENUM(BlueprintType)
enum class AIState : uint8
{
    Patrol,
    Chase,
    Flee,
    Collect
};

UCLASS(Blueprintable)
class SOFTDESIGNTRAINING_API UStateMachine : public UObject
{
    GENERATED_BODY()

public:
    void Run(APawn* AIPawn, FVector& OutDirection);

private:
    AIState CurrentState = AIState::Patrol;
    FVector ClosestPickupPosition = FVector::ZeroVector;

    bool IsCharacterClose(ACharacter* TargetCharacter, APawn* AIPawn);
    bool IsCharacterInSight(ACharacter* TargetCharacter, APawn* AIPawn);

    void FindPickup(APawn* AIPawn);
    void Transition(APawn* AIPawn);

    FVector Chase(APawn* AIPawn);
    FVector Flee(APawn* AIPawn);
    FVector Collect(APawn* AIPawn);
    FVector Move(APawn* AIPawn);
};
