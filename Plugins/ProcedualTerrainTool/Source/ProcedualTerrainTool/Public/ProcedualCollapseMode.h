// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "TerrainHandler.h"

#include "ProcedualCollapseMode.generated.h"

/**
 * A mode determining how superpositions are collapsed.
 */
UCLASS(EditInlineNew, Abstract)
class PROCEDUALTERRAINTOOL_API UProcedualCollapseMode : public UObject
{
	GENERATED_BODY()
	
public:
	/** 
	 * Gets the next super position to collapse on the given shape.
	 * 
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	virtual bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileData> SpawnableTiles) const;
};

/**
 * Collapses 1 superposition at a time at a given location.
 */
UCLASS()
class PROCEDUALTERRAINTOOL_API UManualCollapseMode : public UProcedualCollapseMode
{
	GENERATED_BODY()

	/** 
	 * Gets the next super position to collapse on the given shape. Will collapse at Collapse Coords.
	 * 
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileData> SpawnableTiles) const override;

	//The location to collapse the superposition at. X = Socket Index, Y = Index of the terrain tile to add at Socket Index, Z = The Socket Index on the selected tile to match to the selected Socket Index.
	UPROPERTY(EditAnywhere)
	FIntVector CollapseCoords = FIntVector();
};

/**
 * Collapses superpositions until a circle of a given radius is filled.
 */
UCLASS()
class PROCEDUALTERRAINTOOL_API UCircularCollapseMode : public UProcedualCollapseMode
{
	GENERATED_BODY()

	/** 
	 * Gets the next super position to collapse on the given shape. Will all superpositions within Radius.
	 * 
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileData> SpawnableTiles) const override;

	//The radius of the circle to fill.
	UPROPERTY(EditAnywhere)
	float Radius = 1000;
};

/**
 * Collapses superpositions until a rectangle of a given bounds is filled.
 */
UCLASS()
class PROCEDUALTERRAINTOOL_API URectangularCollapseMode : public UProcedualCollapseMode
{
	GENERATED_BODY()

	/** 
	 * Gets the next super position to collapse on the given shape. Will all superpositions within a box with the given extent.
	 * 
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileData> SpawnableTiles) const override;

	//The extent of the box to fill.
	UPROPERTY(EditAnywhere)
	FVector2D Extent = FVector2D(2000, 1000);
};