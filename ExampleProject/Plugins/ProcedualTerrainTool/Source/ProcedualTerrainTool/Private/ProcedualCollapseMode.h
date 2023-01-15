// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "TerrainGenerator.h"

#include "ProcedualCollapseMode.generated.h"

/* \/ ======================= \/ *\
|  \/ UProcedualCollapseMode  \/  |
\* \/ ======================= \/ */

/**
 * A mode determining how superpositions are collapsed.
 */
UCLASS(CollapseCategories, EditInlineNew, Abstract)
class PROCEDUALTERRAINTOOL_API UProcedualCollapseMode : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * Initializes the terrain transform.
	 */
	UProcedualCollapseMode();

	/** 
	 * Gets the next super position to collapse on the given shape.
	 * 
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	virtual bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles, FRandomStream& RandomStream);

	/** 
	 * Draws the bounds of what will be generated by this collapse mode.
	 * 
	 * @param TerrainTransform - The transform to apply to the bounds.
	 */
	virtual void DrawGenerationBounds() const;

	//The transform of the terrain this is collapsing.
	UPROPERTY();
	FTransform TerrainTransform = FTransform();

	//The location of any errors. Will be 0,0,0 if there are no errors.
	UPROPERTY(Transient)
	FVector ErrorLocation = FVector::ZeroVector;
};

/* /\ ======================= /\ *\
|  /\ UProcedualCollapseMode  /\  |
\* /\ ======================= /\ */



/* \/ ================================== \/ *\
|  \/ AManualCollapseModeLocationMarker  \/  |
\* \/ ================================== \/ */

/**
 * Used to select the location to collapse for the manual collapse mode.
 */
UCLASS(Transient)
class PROCEDUALTERRAINTOOL_API AManualCollapseModeLocationMarker : public AActor
{
	GENERATED_BODY()
public:
	//The connection mode that this is marking.
	UManualCollapseMode* ConnectedMode;

	/**
	 * Places a tile at the location of this.
	 */
	UFUNCTION(CallInEditor)
	void PlaceTile();

	/**
	 * Sets up child actor component and invalidation timer.
	 */
	AManualCollapseModeLocationMarker();
private:
	/**
	 * Checks to see if this should still exist and updates the child actor.
	 */
	UFUNCTION()
	void CheakForInvalidMode();
};

/* /\ ================================== /\ *\
|  /\ AManualCollapseModeLocationMarker  /\  |
\* /\ ================================== /\ */



/* \/ ==================== \/ *\
|  \/ UManualCollapseMode  \/  |
\* \/ ==================== \/ */

/**
 * Collapses 1 superposition at a time at a given location.
 */
UCLASS(Meta = (DisplayName = "Manual"))
class PROCEDUALTERRAINTOOL_API UManualCollapseMode : public UProcedualCollapseMode
{
	GENERATED_BODY()

	/**
	 * Initializes the terrain transform && Spawns the CollapseLocationMarker.
	 */
	UManualCollapseMode();

public:
	/**
	 * Gets the next super position to collapse on the given shape. Will collapse at the location of the CollapseLocationMarker.
	 *
	 * @param SuperPositionIndex - Set to the indices of the super position to collapse next.
	 * @param CurrentShape - The current shape of the terrain.
	 * @param SuperPositions - The current superposition states of the terrain.
	 * @param SpawnableTiles - The tiles that can be spawned.
	 * @return Whether or not another collapse is needed.
	 */
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles, FRandomStream& RandomStream) override;

	//The location to collapse the superposition at.
	UPROPERTY(VisibleAnywhere, Meta = (Category = "Generation Mode Settings", MakeEditWidget = "true"))
	AManualCollapseModeLocationMarker* CollapseLocationMarker;

	//The index of the tile to add.
	UPROPERTY(EditAnywhere, Meta = (Category = "Generation Mode Settings"))
	int TileIndex = 0;
};

/* /\ ==================== /\ *\
|  /\ UManualCollapseMode  /\  |
\* /\ ==================== /\ */



/* \/ ====================== \/ *\
|  \/ UCircularCollapseMode  \/  |
\* \/ ====================== \/ */

/**
 * Collapses superpositions until a circle of a given radius is filled.
 */
UCLASS(Meta = (DisplayName = "Circular"))
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
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles, FRandomStream& RandomStream) override;

	/**
	 * Draws the bounds of what will be generated by this collapse mode.
	 *
	 * @param TerrainTransform - The transform to apply to the bounds.
	 */
	virtual void DrawGenerationBounds() const override;

	//The radius of the circle to fill.
	UPROPERTY(EditAnywhere, Meta = (Category = "Generation Mode Settings"))
	float Radius = 1000;
};

/* /\ ====================== /\ *\
|  /\ UCircularCollapseMode  /\  |
\* /\ ====================== /\ */



/* \/ ========================= \/ *\
|  \/ URectangularCollapseMode  \/  |
\* \/ ========================= \/ */

/**
 * Collapses superpositions until a rectangle of a given bounds is filled.
 */
UCLASS(Meta = (DisplayName = "Rectangular"))
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
	bool GetSuperPositionsToCollapse(FIntVector& SuperPositionIndex, FTerrainShape CurrentShape, TArray<TArray<TArray<bool>>> SuperPositions, TArray<FTerrainTileSpawnData> SpawnableTiles, FRandomStream& RandomStream) override;

	/**
	 * Draws the bounds of what will be generated by this collapse mode.
	 *
	 * @param TerrainTransform - The transform to apply to the bounds.
	 */
	virtual void DrawGenerationBounds() const override;

	//The extent of the box to fill.
	UPROPERTY(EditAnywhere, Meta = (Category = "Generation Mode Settings"))
	FVector2D Extent = FVector2D(2000, 1000);
};

/* /\ ========================= /\ *\
|  /\ URectangularCollapseMode  /\  |
\* /\ ========================= /\ */