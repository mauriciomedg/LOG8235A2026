// Fill out your copyright notice in the Description page of Project Settings.

#include "StateMachine.h"

#include "SoftDesignTraining/SDTCollectible.h"
#include "SoftDesignTraining/SoftDesignTrainingMainCharacter.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

bool UStateMachine::IsCharacterClose(ACharacter* TargetCharacter, APawn* AIPawn)
{
    if (!TargetCharacter || !AIPawn)
    {
        return false;
    }

    const FVector AIPosition = AIPawn->GetActorLocation();
    const FVector PlayerPosition = TargetCharacter->GetActorLocation();
    const float SphereRadius = 800.0f;

    return FVector::Dist(AIPosition, PlayerPosition) <= SphereRadius;
}

bool UStateMachine::IsCharacterInSight(ACharacter* TargetCharacter, APawn* AIPawn)
{
    if (!TargetCharacter || !AIPawn)
    {
        return false;
    }

    const FVector AIPosition = AIPawn->GetActorLocation();
    const FVector PlayerPosition = TargetCharacter->GetActorLocation();

    FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllStaticObjects);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(AIPawn);
    QueryParams.AddIgnoredActor(TargetCharacter);

    FHitResult HitResult;
    const bool bHit = GetWorld()->LineTraceSingleByObjectType(
        HitResult, AIPosition, PlayerPosition, ObjectQueryParams, QueryParams);

    return !bHit;
}

void UStateMachine::FindPickup(APawn* AIPawn)
{
    ClosestPickupPosition = FVector::ZeroVector;

    if (!AIPawn)
    {
        return;
    }

    const FVector AIPosition = AIPawn->GetActorLocation();
    float MinDistance = 750.0f;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        ASDTCollectible* Pickup = Cast<ASDTCollectible>(*It);
        if (!Pickup || Pickup->IsOnCooldown())
        {
            continue;
        }

        const FVector PickupPosition = Pickup->GetActorLocation();
        const float Distance = FVector::Dist(AIPosition, PickupPosition);
        if (Distance >= MinDistance)
        {
            continue;
        }

        // Le pickup doit être visible
        FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(AIPawn);

        FHitResult HitResult;
        const bool bHit = GetWorld()->LineTraceSingleByObjectType(
            HitResult, AIPosition, PickupPosition, ObjectQueryParams, QueryParams);

        if (bHit && HitResult.GetActor() == Pickup)
        {
            ClosestPickupPosition = PickupPosition;
            MinDistance = Distance;
        }
    }
}

FVector UStateMachine::Chase(APawn* AIPawn)
{
    if (!AIPawn)
    {
        return FVector::ZeroVector;
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter)
    {
        return FVector::ZeroVector;
    }

    FVector Direction = PlayerCharacter->GetActorLocation() - AIPawn->GetActorLocation();
    Direction.Z = 0.0f;

    return Direction.GetSafeNormal();
}

FVector UStateMachine::Flee(APawn* AIPawn)
{
    if (!AIPawn)
    {
        return FVector::ZeroVector;
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter)
    {
        return FVector::ZeroVector;
    }

    FVector Direction = AIPawn->GetActorLocation() - PlayerCharacter->GetActorLocation();
    Direction.Z = 0.0f;

    return Direction.GetSafeNormal();
}

FVector UStateMachine::Collect(APawn* AIPawn)
{
    if (!AIPawn || ClosestPickupPosition.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    FVector Direction = ClosestPickupPosition - AIPawn->GetActorLocation();
    Direction.Z = 0.0f;

    return Direction.GetSafeNormal();
}

void UStateMachine::Transition(APawn* AIPawn)
{
    if (!AIPawn)
    {
        return;
    }

    ASoftDesignTrainingMainCharacter* PlayerCharacter =
        Cast<ASoftDesignTrainingMainCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!PlayerCharacter)
    {
        return;
    }

    const bool bPlayerClose = IsCharacterClose(PlayerCharacter, AIPawn);
    const bool bPlayerPoweredUp = PlayerCharacter->IsPoweredUp();

    switch (CurrentState)
    {
    case AIState::Patrol:
        if (bPlayerClose && bPlayerPoweredUp)
        {
            CurrentState = AIState::Flee;
        }
        else if (bPlayerClose)
        {
            CurrentState = AIState::Chase;
        }
        else
        {
            FindPickup(AIPawn);
            if (!ClosestPickupPosition.IsNearlyZero())
            {
                CurrentState = AIState::Collect;
            }
        }
        break;

    case AIState::Chase:
        if (bPlayerClose && bPlayerPoweredUp)
        {
            CurrentState = AIState::Flee;
        }
        else if (!bPlayerClose)
        {
            CurrentState = AIState::Patrol;
        }
        break;

    case AIState::Flee:
        if (!bPlayerClose)
        {
            CurrentState = AIState::Patrol;
        }
        else if (!bPlayerPoweredUp)
        {
            CurrentState = AIState::Chase;
        }
        break;

    case AIState::Collect:
        if (bPlayerClose && bPlayerPoweredUp)
        {
            CurrentState = AIState::Flee;
        }
        else if (bPlayerClose)
        {
            CurrentState = AIState::Chase;
        }
        else
        {
            FindPickup(AIPawn);
            if (ClosestPickupPosition.IsNearlyZero())
            {
                CurrentState = AIState::Patrol;
            }
        }
        break;

    default:
        CurrentState = AIState::Patrol;
        break;
    }
}

FVector UStateMachine::Move(APawn* AIPawn)
{
    if (!AIPawn)
    {
        return FVector::ZeroVector;
    }

    switch (CurrentState)
    {
    case AIState::Patrol:
        return AIPawn->GetActorForwardVector().GetSafeNormal();
    case AIState::Chase:
        return Chase(AIPawn);
    case AIState::Flee:
        return Flee(AIPawn);
    case AIState::Collect:
        return Collect(AIPawn);
    default:
        return FVector::ZeroVector;
    }
}

void UStateMachine::Run(APawn* AIPawn, FVector& OutDirection)
{
    OutDirection = FVector::ZeroVector;

    if (!AIPawn)
    {
        return;
    }

    Transition(AIPawn);
    OutDirection = Move(AIPawn);
}
