// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcedualCollapseMode.h"

#include "Components/ChildActorComponent.h"
#include "TerrainTileData.h"

UProcedualCollapseMode::UProcedualCollapseMode()
{
	AActor* OwningActor = Cast<AActor>(GetOuter());
	UE_LOG(LogTemp, Warning, TEXT("Owner = %s"), *GetOuter()->GetFName().ToString());
	if (IsValid(OwningActor))
	{
		TerrainTransform = OwningActor->GetActorTransform();
	}
}

/**
 * Gets the next super position to collapse on the given shape.
 *
 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
 * @param CurrentShape - The current shape of the terrain.
 * @param SuperPositions - The current superposition states of the terrain.
 * @return Whether or not another collapse is needed.
 */
bool UProcedualCollapseMode::GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles) const
{
	SuperPositionIndex = FIntVector();
	return false;
}

/**
 * Draws the bounds of what will be generated by this collapse mode.
 *
 * @param TerrainTransform - The transform to apply to the bounds.
 */
void UProcedualCollapseMode::DrawGenerationBounds() const
{
	FlushPersistentDebugLines(GetWorld());
}

AManualCollapseModeLocationMarker::AManualCollapseModeLocationMarker()
{
	RootComponent = CreateDefaultSubobject<UChildActorComponent>(FName("Root"));
	RootComponent->SetVisibility(true, true);

	SetActorHiddenInGame(true);

	if (IsValid(GetWorld()))
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AManualCollapseModeLocationMarker::CheakForInvalidMode, 1, true);
	}
}

void AManualCollapseModeLocationMarker::CheakForInvalidMode()
{
	if (!(IsValid(Owner) && IsValid(ConnectedMode)))
	{
		Destroy();
	}
	else
	{
		TSubclassOf<AActor> TargetClass = Cast<ATerrainHandler>(Owner)->SpawnableTiles[ConnectedMode->TileIndex].TileData->ActorClass;
		if (Cast<UChildActorComponent>(RootComponent)->GetChildActorClass() != TargetClass)
		{
			Cast<UChildActorComponent>(RootComponent)->SetChildActorClass(TargetClass);
		}
	}
}

UManualCollapseMode::UManualCollapseMode()
{
	if(IsValid(GetWorld()))
	{
		FActorSpawnParameters SpawnParams = FActorSpawnParameters();
		SpawnParams.Owner = Cast<AActor>(GetOuter());
		CollapseLocation = GetWorld()->SpawnActor<AManualCollapseModeLocationMarker>(TerrainTransform.GetTranslation(), TerrainTransform.GetRotation().Rotator(), SpawnParams);
		CollapseLocation->ConnectedMode = this;
	}
}

/**
 * Gets the next super position to collapse on the given shape. Will collapse at Collapse Coords.
 *
 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
 * @param CurrentShape - The current shape of the terrain.
 * @param SuperPositions - The current superposition states of the terrain.
 * @return Whether or not another collapse is needed.
 */
bool UManualCollapseMode::GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles) const
{
	if (!SuperPositions.IsEmpty() && !CurrentShape.ShapeSockets.IsEmpty())
	{
		//Get socket closest to center
		TMap<int, TArray<int>> PossibleCollapses = TMap<int, TArray<int>>();
		int SocketIndex = 0;

		UE_LOG(LogTemp, Warning, TEXT("CollapseLocation->GetActorLocation() = %s"), *CollapseLocation->GetActorLocation().ToString());
		float ClosestDistanceSquared = FVector2D::DistSquared((CurrentShape.Vertices[SocketIndex] + CurrentShape.Vertices[(SocketIndex + 1) % CurrentShape.Num()]) / 2, FVector2D(TerrainTransform.InverseTransformPosition(CollapseLocation->GetActorLocation())));
		for (int SearchIndex = 1; SearchIndex < CurrentShape.Num(); SearchIndex++)
		{
			float SeachDistanceSquared = ((CurrentShape.Vertices[SearchIndex] + CurrentShape.Vertices[(SearchIndex + 1) % CurrentShape.Num()]) / 2).SquaredLength();
			if (SeachDistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = SeachDistanceSquared;
				SocketIndex = SearchIndex;
			}
		}

		//Get possible collapses around selected socket
		for (int ShapeIndex = 0; ShapeIndex < SuperPositions[SocketIndex].Num(); ShapeIndex++)
		{
			for (int FaceIndex = 0; FaceIndex < SuperPositions[SocketIndex][ShapeIndex].Num(); FaceIndex++)
			{
				if (SuperPositions[SocketIndex][ShapeIndex][FaceIndex])
				{
					if (!PossibleCollapses.Contains(ShapeIndex))
					{
						PossibleCollapses.Emplace(ShapeIndex, TArray<int>());
					}

					PossibleCollapses.Find(ShapeIndex)->Emplace(FaceIndex);
				}
			}
		}

		//End if no valid collapses
		if (PossibleCollapses.IsEmpty())
		{
			FVector2D ErrorLocation = ((CurrentShape.Vertices[SocketIndex] + CurrentShape.Vertices[(SocketIndex + 1) % CurrentShape.Num()]) / 2);
			//DrawDebugPoint(GetWorld(), TerrainTransform.TransformPosition(FVector(ErrorLocation, 0)), 50, FColor::Red, true);
			SuperPositionIndex = FIntVector(0, 0, 0);
			return false;
		}

		//Collapse superposition
		int ShapeIndex = FMath::Clamp(TileIndex, 0, CurrentShape.Num());
		if (ensure(!PossibleCollapses.IsEmpty() && !PossibleCollapses.FindRef(ShapeIndex).IsEmpty()))
		{
			SuperPositionIndex = FIntVector(SocketIndex, ShapeIndex, PossibleCollapses.FindRef(ShapeIndex)[FMath::RandHelper(PossibleCollapses.FindRef(ShapeIndex).Num())]);
			return false;
		}

		//Fail for memory loss
		SuperPositionIndex = FIntVector(0, 0, 0);
		return false;
	}
	//Fail for invalid shapes
	SuperPositionIndex = FIntVector(0, 0, 0);
	return CurrentShape.ShapeSockets.IsEmpty() && !SpawnableTiles.IsEmpty() && !SuperPositions.IsEmpty() && !SuperPositions[0].IsEmpty() && !SuperPositions[0][0].IsEmpty();
	return false;
}

/**
 * Gets the next super position to collapse on the given shape. Will all superpositions within Radius.
 *
 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
 * @param CurrentShape - The current shape of the terrain.
 * @param SuperPositions - The current superposition states of the terrain.
 * @return Whether or not another collapse is needed.
 */
bool UCircularCollapseMode::GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles) const
{
	if (!SuperPositions.IsEmpty() && !CurrentShape.ShapeSockets.IsEmpty())
	{
		//Get socket closest to center
		TMap<int, TArray<int>> PossibleCollapses = TMap<int, TArray<int>>();
		int SocketIndex = 0;

		float ClosestDistanceSquared = ((CurrentShape.Vertices[SocketIndex] + CurrentShape.Vertices[(SocketIndex + 1) % CurrentShape.Num()]) / 2).SquaredLength();
		for (int SearchIndex = 1; SearchIndex < CurrentShape.Num(); SearchIndex++)
		{
			float SeachDistanceSquared = ((CurrentShape.Vertices[SearchIndex] + CurrentShape.Vertices[(SearchIndex + 1) % CurrentShape.Num()]) / 2).SquaredLength();
			if (SeachDistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = SeachDistanceSquared;
				SocketIndex = SearchIndex;
			}
		}

		//Get possible collapses around selected socket
		for (int ShapeIndex = 0; ShapeIndex < SuperPositions[SocketIndex].Num(); ShapeIndex++)
		{
			for (int FaceIndex = 0; FaceIndex < SuperPositions[SocketIndex][ShapeIndex].Num(); FaceIndex++)
			{
				if (SuperPositions[SocketIndex][ShapeIndex][FaceIndex])
				{
					if (!PossibleCollapses.Contains(ShapeIndex))
					{
						PossibleCollapses.Emplace(ShapeIndex, TArray<int>());
					}

					PossibleCollapses.Find(ShapeIndex)->Emplace(FaceIndex);
				}
			}
		}

		//End if no valid collapses
		if (PossibleCollapses.IsEmpty())
		{
			FVector2D ErrorLocation = ((CurrentShape.Vertices[SocketIndex] + CurrentShape.Vertices[(SocketIndex + 1) % CurrentShape.Num()]) / 2);
			//DrawDebugPoint(GetWorld(), TerrainTransform.TransformPosition(FVector(ErrorLocation, 0)), 50, FColor::Red, true);
			SuperPositionIndex = FIntVector(0, 0, 0);
			return false;
		}

		//Get shape index weighted
		TArray<int> Keys;
		PossibleCollapses.GenerateKeyArray(Keys);
		TArray<float> Weights = TArray<float>();
		float WeightSum = 0;
		for (int EachKey : Keys)
		{
			float LastWeight = 0;
			if (Weights.IsValidIndex(Weights.Num() - 1))
			{
				LastWeight = Weights[Weights.Num() - 1];
			}
			WeightSum += SpawnableTiles[EachKey].SpawnWeight;
			Weights.Emplace(SpawnableTiles[EachKey].SpawnWeight + LastWeight);
		}

		int ShapeIndex = 0;
		float RandomSelector = FMath::RandRange(0.f, WeightSum);
		for (int KeyIndex = 0; KeyIndex < Keys.Num(); KeyIndex++)
		{
			if (Weights[KeyIndex] >= RandomSelector)
			{
				ShapeIndex = Keys[KeyIndex];
				break;
			}
		}

		//Collapse superposition
		if (ensure(!PossibleCollapses.IsEmpty() && !PossibleCollapses.FindRef(ShapeIndex).IsEmpty()))
		{
			SuperPositionIndex = FIntVector(SocketIndex, ShapeIndex, PossibleCollapses.FindRef(ShapeIndex)[FMath::RandHelper(PossibleCollapses.FindRef(ShapeIndex).Num())]);
			return ClosestDistanceSquared < Radius * Radius;
		}

		//Fail for memory loss
		SuperPositionIndex = FIntVector(0, 0, 0);
		return false;
	}
	//Fail for invalid shapes
	SuperPositionIndex = FIntVector(0,0,0);
	return CurrentShape.ShapeSockets.IsEmpty() && !SpawnableTiles.IsEmpty() && !SuperPositions.IsEmpty() && !SuperPositions[0].IsEmpty() && !SuperPositions[0][0].IsEmpty();
}

/**
 * Draws the bounds of what will be generated by this collapse mode.
 *
 * @param TerrainTransform - The transform to apply to the bounds.
 */
 void UCircularCollapseMode::DrawGenerationBounds() const
{
	 FlushPersistentDebugLines(GetWorld());
	 DrawDebugCircle(GetWorld(), TerrainTransform.GetTranslation(), Radius, 64, FColor::Magenta, true, 10, 0U, 150, TerrainTransform.GetRotation().GetForwardVector(), TerrainTransform.GetRotation().GetRightVector(), false);
}

/**
 * Gets the next super position to collapse on the given shape. Will all superpositions within a box with the given extent.
 *
 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
 * @param CurrentShape - The current shape of the terrain.
 * @param SuperPositions - The current superposition states of the terrain.
 * @param SpawnableTiles - The tiles that can be spawned.
 * @return Whether or not another collapse is needed.
 */
bool URectangularCollapseMode::GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles) const
{
	if (!SuperPositions.IsEmpty() && !CurrentShape.ShapeSockets.IsEmpty())
	{
		//Find left most point in extent.
		TMap<int, TArray<int>> PossibleCollapses = TMap<int, TArray<int>>();
		int SocketIndex = 0;
		float LeastXValue = MAX_FLT;
		bool bValidSocketFound = false;

		for (int SearchIndex = 0; SearchIndex < CurrentShape.Num(); SearchIndex++)
		{
			FVector2D SocketLocation = ((CurrentShape.Vertices[SearchIndex] + CurrentShape.Vertices[(SearchIndex + 1) % CurrentShape.Num()]) / 2).GetAbs();
			if (SocketLocation.X < LeastXValue && ((SocketLocation.X < abs(Extent.X)) && (SocketLocation.Y < abs(Extent.Y))))
			{
				bValidSocketFound = true;
				LeastXValue = SocketLocation.X;
				SocketIndex = SearchIndex;
			}
		}

		//Get Possible collapses
		for (int ShapeIndex = 0; ShapeIndex < SuperPositions[SocketIndex].Num(); ShapeIndex++)
		{
			for (int FaceIndex = 0; FaceIndex < SuperPositions[SocketIndex][ShapeIndex].Num(); FaceIndex++)
			{
				if (SuperPositions[SocketIndex][ShapeIndex][FaceIndex])
				{
					if (!PossibleCollapses.Contains(ShapeIndex))
					{
						PossibleCollapses.Emplace(ShapeIndex, TArray<int>());
					}

					PossibleCollapses.Find(ShapeIndex)->Emplace(FaceIndex);
				}
			}
		}

		//End if no valid collapses
		if (PossibleCollapses.IsEmpty())
		{
			UE_LOG(LogTerrainTool, Error, TEXT("Shapes do not tile, Consider adding another shape to fill the gap at the marked point or regenerating the terrain"), SocketIndex);
			FVector2D ErrorLocation = ((CurrentShape.Vertices[SocketIndex] + CurrentShape.Vertices[(SocketIndex + 1) % CurrentShape.Num()]) / 2);
			//DrawDebugPoint(GetWorld(), TerrainTransform.TransformPosition(FVector(ErrorLocation, 0)), 50, FColor::Red, true);
			SuperPositionIndex = FIntVector(0, 0, 0);
			return false;
		}

		//Get shape index weighted
		TArray<int> Keys;
		PossibleCollapses.GenerateKeyArray(Keys);
		TArray<float> Weights = TArray<float>();
		float WeightSum = 0;
		for (int EachKey : Keys)
		{
			float LastWeight = 0;
			if (Weights.IsValidIndex(Weights.Num() - 1))
			{
				LastWeight = Weights[Weights.Num() - 1];
			}
			WeightSum += SpawnableTiles[EachKey].SpawnWeight;
			Weights.Emplace(SpawnableTiles[EachKey].SpawnWeight + LastWeight);
		}

		int ShapeIndex = 0;
		float RandomSelector = FMath::RandRange(0.f, WeightSum);
		for (int KeyIndex = 0; KeyIndex < Keys.Num(); KeyIndex++)
		{
			if (Weights[KeyIndex] >= RandomSelector)
			{
				ShapeIndex = Keys[KeyIndex];
				break;
			}
		}

		//Collapse superposition
		if (ensure(!PossibleCollapses.IsEmpty() && !PossibleCollapses.FindRef(ShapeIndex).IsEmpty()))
		{
			SuperPositionIndex = FIntVector(SocketIndex, ShapeIndex, PossibleCollapses.FindRef(ShapeIndex)[FMath::RandHelper(PossibleCollapses.FindRef(ShapeIndex).Num())]);
			return bValidSocketFound;
		}
		SuperPositionIndex = FIntVector(0, 0, 0);
		return false;
	}
	SuperPositionIndex = FIntVector(0, 0, 0);
	return CurrentShape.ShapeSockets.IsEmpty() && !SpawnableTiles.IsEmpty() && !SuperPositions.IsEmpty() && !SuperPositions[0].IsEmpty() && !SuperPositions[0][0].IsEmpty();
}

/**
 * Draws the bounds of what will be generated by this collapse mode.
 *
 * @param TerrainTransform - The transform to apply to the bounds.
 */
void URectangularCollapseMode::DrawGenerationBounds() const
{
	FlushPersistentDebugLines(GetWorld());
	DrawDebugBox(GetWorld(), TerrainTransform.GetTranslation(), FVector(Extent, 0), TerrainTransform.GetRotation(), FColor::Magenta, true, 10, 0U, 150);
}
