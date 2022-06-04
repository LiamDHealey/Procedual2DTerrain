// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainHandler.h"

#include "ProcedualCollapseMode.h"
#include "DrawDebugHelpers.h"
#include "PaperSpriteComponent.h"
#include "TerrainSpriteData.h"

DEFINE_LOG_CATEGORY(LogTerrainTool);

ATerrainHandler::ATerrainHandler()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATerrainHandler::LogTest()
{
	FlushPersistentDebugLines(GetWorld());
	for (int i = 0; i < CurrentShape.Num(); i++)
	{
		FVector Offset = FMath::RandPointInBox(FBox(FVector(-5, -5, -5), FVector(5, 5, 5)));
		FVector LineStart = GetActorTransform().TransformPosition(FVector(CurrentShape.Vertices[i].X, 0, CurrentShape.Vertices[i].Y));
		FVector LineEnd = GetActorTransform().TransformPosition(FVector(CurrentShape.Vertices[(i + 1) % CurrentShape.Num()].X, 0, CurrentShape.Vertices[(i + 1) % CurrentShape.Num()].Y));
		FColor Color = FColor::MakeRandomColor();

		DrawDebugLine(GetWorld(), LineStart, LineEnd, Color, true, 100, 0U, 5);

		Color = (Color.ReinterpretAsLinear() / 2).ToFColor(false);
		for (int j = 1; j <= i; j++)
		{
			if (j % 5 != 0)
			{
				DrawDebugLine(GetWorld(), (LineStart + LineEnd) / 2 + FVector(j * 5, 10, 0) + Offset, (LineStart + LineEnd) / 2 + FVector(j * 5, 10, 10) + Offset, Color, true, 100, 0U, 3);
			}
			else
			{
				DrawDebugLine(GetWorld(), (LineStart + LineEnd) / 2 + FVector((j-4) * 5, 10, 0) + Offset, (LineStart + LineEnd) / 2 + FVector((j-1) * 5, 10, 10) + Offset, Color, true, 100, 0U, 1.5);
			}
		}
	}
}


void ATerrainHandler::RefreshTileSet()
{
	SpriteShapes.Empty();
	BaseSuperPositions.Empty();
	CurrentSpawnableTiles = SpawnableTiles;

	for (FTerrainTileData EachSpawnableTiles : SpawnableTiles)
	{
		SpriteShapes.Emplace(FTerrainShape(EachSpawnableTiles.SpriteData->Verticies, EachSpawnableTiles.SpriteData->FaceIndices));

		TArray<bool> Faces = TArray<bool>();
		Faces.Init(true, EachSpawnableTiles.SpriteData->Verticies.Num());
		BaseSuperPositions.Emplace(Faces);
	}

	ResetTerrain();
}

void ATerrainHandler::ResetTerrain()
{
	FlushPersistentDebugLines(GetWorld());
	CurrentShape = FTerrainShape();

	SuperPositions.SetNum(1);
	SuperPositions[0] = BaseSuperPositions;	

	TSet<UActorComponent*> Components = GetComponents();
	for (UActorComponent* EachComponent : Components)
	{
		if (EachComponent != RootComponent)
		{
			EachComponent->DestroyComponent();
		}
	}
}

void ATerrainHandler::CollapseSuperPosition()
{
	if (IsValid(CollapseMode))
	{
		bool bAnotherCollapseNesseary = true;
		while (bAnotherCollapseNesseary)
		{
			FIntVector CollapseIndex;
			bAnotherCollapseNesseary = CollapseMode->GetSuperPositionsToCollapse(CollapseIndex, CurrentShape, SuperPositions, CurrentSpawnableTiles, GetActorTransform());
			CollapseSuperPosition(CollapseIndex.X, CollapseIndex.Y, CollapseIndex.Z);
		}
	}
	else
	{
		UE_LOG(LogTerrainTool, Error, TEXT("Select a Collapse Mode"));
	}
}

void ATerrainHandler::CollapseSuperPosition(int SocketIndex, int ShapeIndex, int FaceIndex)
{
	UE_LOG(LogTerrainTool, Log, TEXT("\\/------------------------ %i, %i, %i ------------------------\\/"), SocketIndex, ShapeIndex, FaceIndex);
	if (SuperPositions.IsValidIndex(SocketIndex) && SuperPositions[SocketIndex].IsValidIndex(ShapeIndex) && SuperPositions[SocketIndex][ShapeIndex].IsValidIndex(FaceIndex) && SuperPositions[SocketIndex][ShapeIndex][FaceIndex])
	{
		FTerrainShape NewShape;
		FTerrainShapeMergeResult MergeResult;

		if (ensureAlwaysMsgf(CurrentShape.MergeShape(NewShape, MergeResult, SocketIndex, SpriteShapes[ShapeIndex], FaceIndex), TEXT("Super Position Array False")))
		{
			//Calculate Sprite Rotation
			float A;
			float B;
			float C;
			float D;
			MergeResult.Transform.GetMatrix().GetMatrix(A, B, C, D);
			if (FMath::IsNearlyZero(A))
			{
				A = SMALL_NUMBER;
			}

			float Angle = FMath::Atan(B / A);
			if (A < 0)
			{
				Angle = PI + Angle;
			}

			//Spawn Sprite
			FTransform Transform = FTransform(FQuat(FVector(0, -1, 0), Angle), FVector(MergeResult.Transform.GetTranslation().X, 0, MergeResult.Transform.GetTranslation().Y));
			UPaperSpriteComponent* NewSprite = NewObject<UPaperSpriteComponent>(this);
			NewSprite->SetSprite(CurrentSpawnableTiles[ShapeIndex].SpriteData->Sprite);
			NewSprite->RegisterComponent();
			NewSprite->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			NewSprite->SetRelativeTransform(Transform);


			// \/ Refresh Super Positions \/ //

			//Propagate New Super Positions
			TArray<TArray<TArray<bool>>> NewSuperPositions = TArray<TArray<TArray<bool>>>();
			NewSuperPositions.SetNum(NewShape.Num());
			//FString Debug1 = FString();

			for (int Index = 0; Index < NewSuperPositions.Num(); Index++)
			{
				//Debug1 += FString::FromInt(Index) + "<-";
				if (Index < SuperPositions.Num() - MergeResult.Shrinkage)
				{
					//Debug1 += FString::FromInt(UPTTMath::Mod(Index - MergeResult.Offset, SuperPositions.Num())) + ", ";
					NewSuperPositions[Index] = SuperPositions[UPTTMath::Mod(Index - MergeResult.Offset, SuperPositions.Num())];
				}
				else
				{
					//Debug1 += "B, ";
					NewSuperPositions[Index] = BaseSuperPositions;
				}
			}
			SuperPositions = NewSuperPositions;
			CurrentShape = NewShape;

			//UE_LOG(LogTerrainTool, Log, TEXT("%s"), *Debug1);

			//UE_LOG(LogTerrainTool, Log, TEXT("-------------"));
			int NumberOfPossibleCollapses = 0;
			FIntVector CollapseIndex = FIntVector();
			for (int Offset = 0; Offset < MergeResult.Growth + 2; Offset++)
			{
				//FString Debug = FString();
				//Debug += FString::FromInt(UPTTMath::Mod(CurrentShape.Num() - 1 - MergeResult.Growth + Offset, CurrentShape.Num())) + "\t|  ";
				int CollapseSocketIndex = UPTTMath::Mod(CurrentShape.Num() - 1 - MergeResult.Growth + Offset, CurrentShape.Num());
				for (int CollapseShapeIndex = 0; CollapseShapeIndex < BaseSuperPositions.Num(); CollapseShapeIndex++)
				{
					for (int CollapseFaceIndex = 0; CollapseFaceIndex < BaseSuperPositions[CollapseShapeIndex].Num(); CollapseFaceIndex++)
					{
						if (/*SuperPositions[CollapseSocketIndex][CollapseShapeIndex][CollapseFaceIndex]*/true)
						{
							FTerrainShape CollapsedShape;
							FTerrainShapeMergeResult CollapsedShapeMergeResult;
							if (CurrentShape.MergeShape(CollapsedShape, CollapsedShapeMergeResult, CollapseSocketIndex, SpriteShapes[CollapseShapeIndex], CollapseFaceIndex))
							{
								//Debug += "C";
								bool bCanCollapse = HasNewCollapseableSuperPositions(CollapsedShape, CollapsedShapeMergeResult, CollapsePredictionDepth);
								SuperPositions[CollapseSocketIndex][CollapseShapeIndex][CollapseFaceIndex] = bCanCollapse;
								if (bCanCollapse)
								{
									//Debug += "T";
									NumberOfPossibleCollapses++;
									CollapseIndex = FIntVector(CollapseSocketIndex, CollapseShapeIndex, CollapseFaceIndex);
								}
								else
								{
									//Debug += "F";
								}
							}
							else
							{
								//Debug += " F";
								SuperPositions[CollapseSocketIndex][CollapseShapeIndex][CollapseFaceIndex] = false;
							}
						}
						else
						{
							//Debug += "  ";
						}
						//Debug += ", ";
					}
					//Debug += "  |  ";
				}
				//UE_LOG(LogTerrainTool, Log, TEXT("%s"), *Debug);
			}

			if (NumberOfPossibleCollapses == 1)
			{
				CollapseSuperPosition(CollapseIndex.X, CollapseIndex.Y, CollapseIndex.Z);
			}

			// /\ Refresh Super Positions /\ //
			return;
		}
	}
	UE_LOG(LogTerrainTool, Error, TEXT("Failed"));
}

bool ATerrainHandler::HasNewCollapseableSuperPositions(FTerrainShape Shape, FTerrainShapeMergeResult MergeResult, int SearchDepth)
{
	for (int Offset = 0; Offset < MergeResult.Growth + 2; Offset++)
	{
		int CollapseSocketIndex = UPTTMath::Mod(Shape.Num() - 1 - MergeResult.Growth + Offset, Shape.Num());
		for (int CollapseShapeIndex = 0; CollapseShapeIndex < BaseSuperPositions.Num(); CollapseShapeIndex++)
		{
			for (int CollapseFaceIndex = 0; CollapseFaceIndex < BaseSuperPositions[CollapseShapeIndex].Num(); CollapseFaceIndex++)
			{				
				FTerrainShape CollapsedShape;
				FTerrainShapeMergeResult CollapsedShapeMergeResult;
				if (Shape.MergeShape(CollapsedShape, CollapsedShapeMergeResult, CollapseSocketIndex, SpriteShapes[CollapseShapeIndex], CollapseFaceIndex))
				{
					if (SearchDepth == 0 || HasNewCollapseableSuperPositions(CollapsedShape, CollapsedShapeMergeResult, SearchDepth - 1))
					{
						goto NextSocket;
					}
				}
			}
		}
		return false;


		NextSocket:
		;
	}

	return true;
}