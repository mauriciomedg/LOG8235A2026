// Fill out your copyright notice in the Description page of Project Settings.


#include "StateMachine.h"

#include "StateMachine.h"
#include "SoftDesignTraining/SDTCollectible.h"
#include "EngineUtils.h"
#include "SoftDesignTraining/SoftDesignTrainingMainCharacter.h"
#include "Kismet/GameplayStatics.h"

bool UStateMachine::IsCharacterClose(ACharacter* TargetCharacter, APawn* AIPawn)
{
    FVector AIPosition = AIPawn->GetActorLocation();
    FVector PlayerPosition = TargetCharacter->GetActorLocation();

    float SphereRadius = 800.0f;

    return FVector::Dist(AIPosition, PlayerPosition) <= SphereRadius;
}

bool UStateMachine::IsCharacterInSight(ACharacter* TargetCharacter, APawn* AIPawn)
{
    FVector AIPosition = AIPawn->GetActorLocation();
    FVector PlayerPosition = TargetCharacter->GetActorLocation();

    FHitResult HitResult;
    FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllStaticObjects);

    bool bHit = GetWorld()->LineTraceSingleByObjectType(
        HitResult,
        AIPosition,
        PlayerPosition,
        QueryParams
    );


    if (bHit)
        return false;
    return true;
}

void UStateMachine::FindPickup(APawn* AIPawn)
{
    FVector AIPosition = AIPawn->GetActorLocation();
    float MinDistance = 750.0f;
    ClosestPickupPosition = FVector::ZeroVector;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        ASDTCollectible* Pickup = Cast<ASDTCollectible>(*It);
        if (Pickup && !Pickup->IsOnCooldown())
        {
            FVector PickupPosition = Pickup->GetActorLocation();
            float Distance = FVector::Dist(AIPosition, PickupPosition);

            if (Distance < MinDistance)
            {
                FHitResult HitResult;
                FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);
                FCollisionQueryParams QueryParams;
                QueryParams.AddIgnoredActor(AIPawn);

                bool bHit = GetWorld()->LineTraceSingleByObjectType(
                    HitResult,
                    AIPosition,
                    PickupPosition,
                    ObjectQueryParams,
                    QueryParams
                );
                //UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), bHit ? *HitResult.GetActor()->GetName() : TEXT("None"));
                if (bHit && HitResult.GetActor() == Pickup)
                {
                    ClosestPickupPosition = PickupPosition;
                }
            }
        }
    }
}

void UStateMachine::UpdateReferencePosition(ACharacter* TargetCharacter, APawn* AIPawn)
{
    FVector PlayerPosition = TargetCharacter->GetActorLocation();
    if (IsCharacterInSight(TargetCharacter, AIPawn) && FVector::Dist(ReferencePlayerPosition, PlayerPosition) > 10.0f)
        ReferencePlayerPosition = PlayerPosition;
}

FVector UStateMachine::Chase(APawn* AIPawn)
{
    ACharacter* PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    UpdateReferencePosition(PlayerCharacter, AIPawn);
    FVector AIPosition = AIPawn->GetActorLocation();
    FVector DirectionVector = (ReferencePlayerPosition - AIPosition).GetSafeNormal();
    //AddMovement(DirectionVector);
    return DirectionVector;
}

FVector UStateMachine::Flee(APawn* AIPawn)
{
    ACharacter* PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    FVector AIPosition = AIPawn->GetActorLocation();
    FVector PlayerPosition = PlayerCharacter->GetActorLocation();
    FVector DirectionVector = (PlayerPosition - AIPosition).GetSafeNormal();
    //AddMovement(-DirectionVector);
    return -DirectionVector;
}

FVector UStateMachine::Collect(APawn* AIPawn)
{
    FVector AIPosition = AIPawn->GetActorLocation();
    FVector DirectionVector = (ClosestPickupPosition - AIPosition).GetSafeNormal();
    //AddMovement(DirectionVector);
    return DirectionVector;
}

void UStateMachine::Transition(APawn* AIPawn)
{
    ASoftDesignTrainingMainCharacter* PlayerCharacter = Cast<ASoftDesignTrainingMainCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    bool bIsClose = IsCharacterClose(PlayerCharacter, AIPawn);
    bool bInSight = IsCharacterInSight(PlayerCharacter, AIPawn);
    bool bplayerHasPickup = PlayerCharacter->IsPoweredUp();

    switch (CurrentState)
    {
    case AIState::Patrol:
        if (bIsClose && bInSight && !bplayerHasPickup)
        {
            CurrentState = AIState::Chase;
        }
        if (bIsClose && bInSight && bplayerHasPickup)
        {
            CurrentState = AIState::Flee;
        }
        FindPickup(AIPawn);
        if (ClosestPickupPosition != FVector::ZeroVector)
        {
            CurrentState = AIState::Collect;
        }
        break;
    case AIState::Chase:
        if (!bIsClose)
        {
            CurrentState = AIState::Patrol;
        }
        if (bIsClose && bInSight && bplayerHasPickup)
        {
            CurrentState = AIState::Flee;
        }
        break;
    case AIState::Flee:
        if (!bIsClose)
        {
            CurrentState = AIState::Patrol;
        }
        if (bIsClose && bInSight && !bplayerHasPickup)
        {
            CurrentState = AIState::Chase;
        }
        break;
    case AIState::Collect:
        if (bIsClose && bInSight && !bplayerHasPickup)
        {
            CurrentState = AIState::Chase;
        }
        if (bIsClose && bInSight && bplayerHasPickup)
        {
            CurrentState = AIState::Flee;
        }
        FindPickup(AIPawn);
        if (ClosestPickupPosition == FVector::ZeroVector)
        {
            CurrentState = AIState::Patrol;
        }
        break;
    default:
        break;
    }
}

FVector UStateMachine::Move(APawn* AIPawn)
{
    FVector Direction = FVector::ZeroVector;

    switch (CurrentState)
    {
    case AIState::Patrol:
        Direction = AIPawn->GetActorForwardVector();
        break;
    case AIState::Chase:
        Direction = Chase(AIPawn);
        break;
    case AIState::Flee:
        Direction = Flee(AIPawn);
        break;
    case AIState::Collect:
        Direction = Collect(AIPawn);
        break;
    default:
        break;
    }

    return Direction;
}

void UStateMachine::Run(APawn* AIPawn, FVector& OutDirection)
{
    if (AIPawn)
    {
        Transition(AIPawn);
        OutDirection = Move(AIPawn);

        FVector AIPosition = AIPawn->GetActorLocation();
    }
}
