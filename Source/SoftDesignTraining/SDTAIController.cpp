// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"
#include "StateMachine.h"
#include "SDTUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

ASDTAIController::ASDTAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    StateMachine = CreateDefaultSubobject<UStateMachine>(TEXT("AIStateMachine"));
}

void ASDTAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    FVector DesiredDirection = FVector::ZeroVector;
    StateMachine->Run(ControlledPawn, DesiredDirection);

    Navigation(DesiredDirection, DeltaTime);
}

void ASDTAIController::Navigation(const FVector& DesiredDirection, float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
    if (!ControlledCharacter)
    {
        return;
    }

    UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement();
    if (!MovementComponent)
    {
        return;
    }

    FVector DesiredMovementDirection = DesiredDirection.GetSafeNormal();
    DesiredMovementDirection.Z = 0.0f;
    DesiredMovementDirection.Normalize();

    if (DesiredMovementDirection.IsNearlyZero())
    {
        Velocity = FVector::ZeroVector;
        MovementComponent->Velocity = FVector::ZeroVector;
        return;
    }

    MovementComponent->MaxWalkSpeed = MaxSpeed;

    // Death floor sur le chemin direct : on commence l'évitement
    TArray<FHitResult> DirectDeathFloorHits;
    const bool bDirectPathBlockedByDeathFloor = DetectDeathFloor(DesiredMovementDirection, DirectDeathFloorHits);

    if (bDirectPathBlockedByDeathFloor && !bAvoidingDeathFloor)
    {
        bAvoidingDeathFloor = true;
        DeathFloorAvoidanceDirection = ChooseDeathFloorAvoidanceDirection(DesiredMovementDirection);
        DeathFloorAvoidanceDirection = MakeDeathFloorDirectionSafe(DeathFloorAvoidanceDirection, DirectDeathFloorHits);
    }

    FVector MovementDirection = DesiredMovementDirection;

    // Évitement du death floor en cours
    if (bAvoidingDeathFloor)
    {
        TArray<FHitResult> NewDirectDeathFloorHits;
        const bool bStillBlocked = DetectDeathFloor(DesiredMovementDirection, NewDirectDeathFloorHits);

        if (!bStillBlocked)
        {
            bAvoidingDeathFloor = false;
            DeathFloorAvoidanceDirection = FVector::ZeroVector;
            MovementDirection = DesiredMovementDirection;
        }
        else
        {
            MovementDirection = MakeDeathFloorDirectionSafe(DeathFloorAvoidanceDirection, NewDirectDeathFloorHits);
            DeathFloorAvoidanceDirection = MovementDirection;

            TArray<FHitResult> EscapeWallHits;
            if (DetectWall(MovementDirection, EscapeWallHits))
            {
                MovementDirection = ComputeWallAvoidanceDirection(MovementDirection, MovementDirection, EscapeWallHits, DeltaTime);
                MovementDirection = MakeDeathFloorDirectionSafe(MovementDirection, NewDirectDeathFloorHits);
                DeathFloorAvoidanceDirection = MovementDirection;
            }
        }
    }

    // Évitement des murs normal
    if (!bAvoidingDeathFloor)
    {
        TArray<FHitResult> WallHits;
        if (DetectWall(MovementDirection, WallHits))
        {
            const FVector CurrentDirection = Velocity.IsNearlyZero() ? MovementDirection : Velocity.GetSafeNormal();
            MovementDirection = ComputeWallAvoidanceDirection(CurrentDirection, MovementDirection, WallHits, DeltaTime);
        }
    }

    // Accélération et vitesse max
    const float CurrentMaxSpeed = bAvoidingDeathFloor ? DeathFloorAvoidanceSpeed : MaxSpeed;
    const float CurrentSpeed = FMath::Clamp(Velocity.Size() + Acceleration * DeltaTime, 0.0f, CurrentMaxSpeed);

    Velocity = MovementDirection.GetSafeNormal() * CurrentSpeed;
    MovementComponent->Velocity = Velocity;

    if (!Velocity.IsNearlyZero())
    {
        ControlledPawn->SetActorRotation(Velocity.GetSafeNormal().Rotation());
    }
}

FVector ASDTAIController::ChooseDeathFloorAvoidanceDirection(const FVector& DesiredDirection) const
{
    const FVector LeftDirection = FVector::CrossProduct(FVector::UpVector, DesiredDirection).GetSafeNormal();
    const FVector RightDirection = -LeftDirection;

    TArray<FHitResult> LeftWallHits, LeftDeathHits, RightWallHits, RightDeathHits;

    const bool bLeftWall = DetectWall(LeftDirection, LeftWallHits);
    const bool bLeftDeath = DetectDeathFloor(LeftDirection, LeftDeathHits);
    const bool bRightWall = DetectWall(RightDirection, RightWallHits);
    const bool bRightDeath = DetectDeathFloor(RightDirection, RightDeathHits);

    const bool bLeftSafe = !bLeftWall && !bLeftDeath;
    const bool bRightSafe = !bRightWall && !bRightDeath;

    if (bLeftSafe && !bRightSafe)
    {
        return LeftDirection;
    }

    if (bRightSafe && !bLeftSafe)
    {
        return RightDirection;
    }

    // Les deux côtés sont sûrs : on garde celui le plus proche de la vélocité actuelle
    if (bLeftSafe && bRightSafe && !Velocity.IsNearlyZero())
    {
        const FVector CurrentDirection = Velocity.GetSafeNormal();
        const float LeftAlignment = FVector::DotProduct(CurrentDirection, LeftDirection);
        const float RightAlignment = FVector::DotProduct(CurrentDirection, RightDirection);

        return LeftAlignment >= RightAlignment ? LeftDirection : RightDirection;
    }

    return LeftDirection;
}

FVector ASDTAIController::MakeDeathFloorDirectionSafe(
    const FVector& ProposedDirection, const TArray<FHitResult>& DeathFloorHits) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || DeathFloorHits.Num() == 0)
    {
        return ProposedDirection.GetSafeNormal();
    }

    const FVector AIPosition = ControlledPawn->GetActorLocation();

    // Hit de death floor le plus proche
    const FHitResult* ClosestHit = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();

    for (const FHitResult& Hit : DeathFloorHits)
    {
        const float DistanceSquared = FVector::DistSquared(AIPosition, Hit.ImpactPoint);
        if (DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestHit = &Hit;
        }
    }

    if (!ClosestHit)
    {
        return ProposedDirection.GetSafeNormal();
    }

    FVector TowardDanger = ClosestHit->ImpactPoint - AIPosition;
    TowardDanger.Z = 0.0f;

    if (TowardDanger.IsNearlyZero())
    {
        return ProposedDirection.GetSafeNormal();
    }

    TowardDanger.Normalize();

    // On retire la composante qui pointe vers le danger
    FVector SafeDirection = ProposedDirection.GetSafeNormal();
    const float TowardDangerAmount = FVector::DotProduct(SafeDirection, TowardDanger);

    if (TowardDangerAmount > 0.0f)
    {
        SafeDirection -= TowardDanger * TowardDangerAmount;
    }

    // Direction pile vers le danger : on prend une tangente
    if (SafeDirection.IsNearlyZero())
    {
        const FVector TangentA = FVector::CrossProduct(FVector::UpVector, TowardDanger).GetSafeNormal();
        const FVector TangentB = -TangentA;

        if (!DeathFloorAvoidanceDirection.IsNearlyZero())
        {
            const float AlignmentA = FVector::DotProduct(TangentA, DeathFloorAvoidanceDirection);
            const float AlignmentB = FVector::DotProduct(TangentB, DeathFloorAvoidanceDirection);
            SafeDirection = AlignmentA >= AlignmentB ? TangentA : TangentB;
        }
        else
        {
            SafeDirection = TangentA;
        }
    }

    SafeDirection.Z = 0.0f;
    return SafeDirection.GetSafeNormal();
}

FVector ASDTAIController::ComputeWallAvoidanceDirection(
    const FVector& CurrentDirection, const FVector& DesiredDirection,
    const TArray<FHitResult>& Hits, float DeltaTime) const
{
    FVector AvoidanceNormal = FVector::ZeroVector;

    for (const FHitResult& Hit : Hits)
    {
        if (!Hit.bBlockingHit)
        {
            continue;
        }

        FVector Normal = Hit.ImpactNormal;
        Normal.Z = 0.0f;

        if (!Normal.IsNearlyZero())
        {
            AvoidanceNormal += Normal.GetSafeNormal();
        }
    }

    if (AvoidanceNormal.IsNearlyZero())
    {
        return CurrentDirection.GetSafeNormal();
    }

    AvoidanceNormal.Normalize();

    const float CrossZ = FVector::CrossProduct(CurrentDirection, AvoidanceNormal).Z;
    float TurnSign = FMath::Sign(CrossZ);

    if (FMath::IsNearlyZero(TurnSign))
    {
        TurnSign = 1.0f;
    }

    const float RotationAmount = TurnSign * WallAvoidanceAngle * DeltaTime;

    FVector NewDirection = CurrentDirection.RotateAngleAxis(RotationAmount, FVector::UpVector);
    NewDirection.Z = 0.0f;

    return NewDirection.GetSafeNormal();
}

bool ASDTAIController::DetectWall(const FVector& Direction, TArray<FHitResult>& Hits) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }

    const FVector NormalizedDirection = Direction.GetSafeNormal();
    if (NormalizedDirection.IsNearlyZero())
    {
        return false;
    }

    const FVector Start = ControlledPawn->GetActorLocation();
    const FVector End = Start + NormalizedDirection * WallDetectionDistance;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledPawn);

    const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(WallSweepRadius);

    const bool bHit = GetWorld()->SweepMultiByObjectType(
        Hits, Start, End, FQuat::Identity, ObjectQueryParams, CollisionShape, QueryParams);

    // DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.0f, 0, 2.0f);

    return bHit;
}

bool ASDTAIController::DetectDeathFloor(const FVector& Direction, TArray<FHitResult>& Hits) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }

    const FVector NormalizedDirection = Direction.GetSafeNormal();
    if (NormalizedDirection.IsNearlyZero())
    {
        return false;
    }

    ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
    if (!ControlledCharacter)
    {
        return false;
    }

    UCapsuleComponent* Capsule = ControlledCharacter->GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();

    // Trois sondes au niveau des pieds : gauche, centre, droite
    FVector BaseStart = ControlledPawn->GetActorLocation();
    BaseStart.Z -= CapsuleHalfHeight - 20.0f;

    const FVector RightVector = FVector::CrossProduct(FVector::UpVector, NormalizedDirection).GetSafeNormal();
    const float SideOffset = CapsuleRadius + 20.0f;

    const FVector ProbeOffsets[] = { -RightVector * SideOffset, FVector::ZeroVector, RightVector * SideOffset };

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(COLLISION_DEATH_OBJECT);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledPawn);

    const float ProbeRadius = 30.0f;
    const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(ProbeRadius);

    bool bDetectedDeathFloor = false;

    for (const FVector& Offset : ProbeOffsets)
    {
        const FVector Start = BaseStart + Offset;
        const FVector End = Start + NormalizedDirection * DeathFloorDetectionDistance;

        TArray<FHitResult> ProbeHits;
        const bool bHit = GetWorld()->SweepMultiByObjectType(
            ProbeHits, Start, End, FQuat::Identity, ObjectQueryParams, CollisionShape, QueryParams);

        const FColor DebugColor = bHit ? FColor::Purple : FColor::Yellow;
        DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 0.0f, 0, 3.0f);
        DrawDebugSphere(GetWorld(), End, ProbeRadius, 12, DebugColor, false, 0.0f);

        if (bHit)
        {
            bDetectedDeathFloor = true;
            Hits.Append(ProbeHits);
        }
    }

    return bDetectedDeathFloor;
}

