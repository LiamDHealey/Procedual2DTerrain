#pragma once

#include "CoreMinimal.h"
#include "TerrainShape.generated.h"

/**
 * A socket's connectivity state.
 */
UENUM(BlueprintType)
enum class EConnectionResult : uint8
{
	No				= 0,
	CheckNext		= 1,
	CheckPrevious	= 2,
	CheckBoth		= 3,
	Yes				= 4,
};

/**
 * A socket storing data relevant for terrain piece connections.
 */
USTRUCT(BlueprintType)
struct PROCEDUALTERRAINTOOL_API FTerrainSocket
{
	GENERATED_BODY()

	//The index of this socket. Will only connect to sockets with the same index. Negative Indices will never connect.
	UPROPERTY(VisibleAnywhere)
	int SocketIndex = 0;

	//The length of this socket. Will only connect to sockets with the same length.
	UPROPERTY(VisibleAnywhere)
	float Length = 1;

	//The angle this socket blocks around its first vertex.
	UPROPERTY(VisibleAnywhere)
	float FirstAngle = 2 * PI;

	//The angle this socket blocks around its second vertex.
	UPROPERTY(VisibleAnywhere)
	float SecondAngle = 2 * PI;

	/**
	 * Constructs a socket given terrain geometry and an index.
	 * 
	 * @param Index - The index of this socket. Will only connect to sockets with the same index. Negative Indices will never connect.
	 * @param PreviousVertex - The vertex prior to the FirstVertex.
	 * @param FirstVertex - The first vertex defining the edge of this socket.
	 * @param SecondVertex - The second vertex defining the edge of this socket.
	 * @param NextVertex - The vertex after the SecondVertex.
	 */
	FTerrainSocket(int Index = -1, FVector2D PreviousVertex = FVector2D(), FVector2D FirstVertex = FVector2D(), FVector2D SecondVertex = FVector2D(), FVector2D NextVertex = FVector2D())
	{
		SocketIndex = Index;
		Length = FVector2D::Distance(FirstVertex, SecondVertex);

		FVector2D FP = (PreviousVertex - FirstVertex).GetSafeNormal();
		FVector2D FS = (SecondVertex - FirstVertex).GetSafeNormal();
		FVector2D SN = (NextVertex - SecondVertex).GetSafeNormal();
		FVector2D SF = (FirstVertex - SecondVertex).GetSafeNormal();
		
		FirstAngle = FMath::Acos(FP | FS);
		//if ((FP ^ FS) < 0)
		//{
		//	FirstAngle = 2 * PI - FirstAngle;
		//}

		SecondAngle = FMath::Acos(SF | SN);
		//if ((SF ^ SN) < 0)
		//{
		//	SecondAngle = 2 * PI - SecondAngle;
		//}
	}

	/**
	 * Increases the first angle.
	 * 
	 * @param Amount - The amount to increase the first angle by.
	 */
	void IncreaseFirstAngle(float Amount)
	{
		FirstAngle += Amount;
	}

	/**
	 * Increases the second angle.
	 *
	 * @param Amount - The amount to increase the second angle by.
	 */
	void IncreaseSecondAngle(float Amount)
	{
		SecondAngle += Amount;
	}

	/**
	 * Determines whether sockets can connect.
	 * 
	 * @param Other - The other socket to test.
	 * @return Whether or not this and Other can connect.
	 */
	EConnectionResult CanConnectToSocket(FTerrainSocket Other) const
	{
		if (SocketIndex < 0 || SocketIndex != Other.SocketIndex || Length != Other.Length || SecondAngle + Other.FirstAngle > 2 * PI || FirstAngle + Other.SecondAngle > 2 * PI)
		{
			return EConnectionResult::No;
		}

		bool bCheckNext = FMath::IsNearlyEqual(SecondAngle + Other.FirstAngle, TWO_PI, .1);
		bool bCheckPrevious = FMath::IsNearlyEqual(FirstAngle + Other.SecondAngle, TWO_PI, .1);

		switch ((bCheckNext)+(2 * bCheckPrevious))
		{
		case 0:
			return EConnectionResult::Yes;

		case 1:
			return EConnectionResult::CheckNext;

		case 2:
			return EConnectionResult::CheckPrevious;

		case 3:
			return EConnectionResult::CheckBoth;

		default:
			return EConnectionResult::No;
		}
	}
};

/**
 * Stores the results of a terrain shape merge.
 */
USTRUCT(BlueprintType)
struct PROCEDUALTERRAINTOOL_API FTerrainShapeMergeResult
{
	GENERATED_BODY()

	//The transform applied to the other shape.
	FTransform2D Transform = FTransform2D();

	//The change of the indices of the original shape.
	UPROPERTY()
	int Offset = 0;
		
	//The number of new vertices gained during the merge.
	UPROPERTY()
	int Growth = 0;
};

/**
 * Stores a piece of terrain's shape and its sockets.
 */
USTRUCT(BlueprintType)
struct PROCEDUALTERRAINTOOL_API FTerrainShape
{
	GENERATED_BODY()

	//Stores all of the sockets in this shape.
	UPROPERTY(VisibleAnywhere)
	TArray<FTerrainSocket> ShapeSockets = TArray<FTerrainSocket>();

	//Stores all of the vertices of this shape.
	UPROPERTY(VisibleAnywhere)
	TArray<FVector2D> Vertices = TArray<FVector2D>();

	//Constructs a terrain shape from the given terrain's geometry.
	FTerrainShape(TArray<FVector2D> TerrainGeometery = TArray<FVector2D>())
	{
		TArray<int> FaceIndices = TArray<int>();
		FaceIndices.SetNumZeroed(TerrainGeometery.Num());
		FTerrainShape(TerrainGeometery, FaceIndices);
	}

	//Constructs a terrain shape from the given terrain's geometry and face indices.
	FTerrainShape(TArray<FVector2D> TerrainGeometery, TArray<int> FaceIndices)
	{
		if (TerrainGeometery.IsEmpty())
		{
			return;
		}

		FaceIndices.SetNumZeroed(TerrainGeometery.Num());
		for (int GeoIndex = 1; GeoIndex <= TerrainGeometery.Num(); GeoIndex++)
		{
			FTerrainSocket NewSocket = FTerrainSocket(FaceIndices[GeoIndex % FaceIndices.Num()], TerrainGeometery[GeoIndex - 1], TerrainGeometery[GeoIndex % TerrainGeometery.Num()], TerrainGeometery[(GeoIndex + 1) % TerrainGeometery.Num()], TerrainGeometery[(GeoIndex + 2) % TerrainGeometery.Num()]);
			ShapeSockets.Emplace(NewSocket);
			Vertices.Emplace(TerrainGeometery[GeoIndex % TerrainGeometery.Num()]);
		}
	}

	//Constructs a terrain shape from the given sockets.
	FTerrainShape(TArray<FVector2D> TerrainGeometery, TArray<FTerrainSocket> Sockets)
	{
		Vertices = TArray<FVector2D>(TerrainGeometery);
		ShapeSockets = TArray<FTerrainSocket>(Sockets);
	}

	/**
	 * Gets the number of sockets this shape has.
	 * 
	 * @return The number of sockets this shape has.
	 */
	int Num() const
	{
		return ShapeSockets.Num();
	}

	/**
	 * Attempts to merge this shape with another.
	 * 
	 * @param MergedShape - The shape of the this and other combined.
	 * @param MergedResult - Data about how the shapes were merged.
	 * @param FaceIndex - The index of the face on this shape to start the merge at.
	 * @param Other - The other shape to query.
	 * @param FaceIndex - The index of the face on this the other shape to start the merge at.
	 * @return Whether or not this shape can be merged with the other shape.
	 */
	bool MergeShape(FTerrainShape& MergedShape, FTerrainShapeMergeResult& MergeResult, int FaceIndex, FTerrainShape Other, int OtherFaceIndex, bool bDebug = false) const
	{
		MergeResult = FTerrainShapeMergeResult();

		//Account for empty shapes.
		if (ShapeSockets.IsEmpty() && !Other.ShapeSockets.IsEmpty())
		{
			MergedShape = FTerrainShape(Other.Vertices, Other.ShapeSockets);
			MergeResult.Transform = FTransform2D();
			MergeResult.Growth = Other.Num();
			return true;
		}

		//Fail Invalid Merges
		if (Other.ShapeSockets.IsEmpty() || !ShapeSockets.IsValidIndex(FaceIndex) || !Other.ShapeSockets.IsValidIndex(OtherFaceIndex))
		{
			MergedShape = FTerrainShape();
			return false;
		}

		// \/ Detect if merge is possible \/ //
		int FirstMergeIndex = FaceIndex;
		int LastMergeIndex = FaceIndex;
		int OtherFirstMergeIndex = OtherFaceIndex;
		int OtherLastMergeIndex = OtherFaceIndex;


		bool bClockwise = false;
		bool bSearchClockwise = true;
		do
		{
			//Reset indices
			
			int SearchIndex = Mod(FaceIndex + bClockwise, Num());
			int OtherSearchIndex = Mod(OtherFaceIndex - bClockwise, Other.Num());

			//Search all of shapes sockets to detect if connection is possible
			do
			{
				//Test socket connectivity
				switch (ShapeSockets[SearchIndex].CanConnectToSocket(Other.ShapeSockets[OtherSearchIndex]))
				{
				case EConnectionResult::No:
					MergedShape = FTerrainShape();
					return false;

				case EConnectionResult::CheckNext:
					if (!bClockwise)
					{
						FirstMergeIndex = SearchIndex;
						OtherLastMergeIndex = OtherSearchIndex;
						if (bSearchClockwise)
						{
							goto nextLoop;
						}
						goto skipLoop;
					}
					break;

				case EConnectionResult::CheckPrevious:
					if (bClockwise)
					{
						LastMergeIndex = SearchIndex;
						OtherFirstMergeIndex = OtherSearchIndex;
						goto nextLoop; 
					}
					bSearchClockwise = false;
					break;

				case EConnectionResult::CheckBoth:
					break;

				case EConnectionResult::Yes:
					if (ensureMsgf(SearchIndex == FaceIndex, TEXT("Improper Shape Angle Detection")))
					{
						goto skipLoop;
					}

					goto nextLoop;

				default:
					ensureMsgf(false, TEXT("Invalid Socket Test Result"));
					break;
				}

				//Iterate Indices
				SearchIndex = Mod(SearchIndex + (bClockwise ? 1 : -1), Num());
				OtherSearchIndex = Mod(OtherSearchIndex + (!bClockwise ? 1 : -1), Other.Num());

			} while (SearchIndex != FaceIndex);
			ensureMsgf(false, TEXT("Shape indices mismatch"));

		nextLoop:
			bClockwise = !bClockwise;

		} while ((bClockwise));
	skipLoop:
		// /\ Detect if merge is possible /\ //
		
		// \/ Merge Shapes \/ //
		
		//Prune Merged Sockets & Vertices
		int MergeLength = (FirstMergeIndex <= LastMergeIndex ? LastMergeIndex - FirstMergeIndex + 1 : LastMergeIndex + Num() - FirstMergeIndex + 1);
		MergedShape = FTerrainShape();
		MergedShape.ShapeSockets.SetNum(Num() - MergeLength);
		MergedShape.Vertices.SetNum(Num() - MergeLength);

		MergeResult.Growth = -1 * MergeLength;
		MergeResult.Offset = -1 * Mod(LastMergeIndex + 1, Num());

		for (int MergeOffset = 0; MergeOffset < MergedShape.Num(); MergeOffset++)
		{
			MergedShape.ShapeSockets[MergeOffset] = ShapeSockets[Mod(LastMergeIndex + 1 + MergeOffset, Num())];
			MergedShape.Vertices[MergeOffset] = Vertices[Mod(LastMergeIndex + 1 + MergeOffset, Num())];
		}


		//Adjust socket angles
		TArray<FTerrainSocket> OtherShapeSockets = TArray<FTerrainSocket>(Other.ShapeSockets);
		OtherShapeSockets[Mod(OtherLastMergeIndex + 1, OtherShapeSockets.Num())].IncreaseFirstAngle(ShapeSockets[FirstMergeIndex].FirstAngle);
		OtherShapeSockets[Mod(OtherFirstMergeIndex - 1, OtherShapeSockets.Num())].IncreaseSecondAngle(ShapeSockets[LastMergeIndex].SecondAngle);

		MergedShape.ShapeSockets[0].IncreaseFirstAngle(Other.ShapeSockets[OtherFirstMergeIndex].FirstAngle);
		MergedShape.ShapeSockets[MergedShape.Num() - 1].IncreaseSecondAngle(Other.ShapeSockets[OtherLastMergeIndex].SecondAngle);


		//Get Other Transform
		FQuat2D Target = FQuat2D((MergedShape.Vertices[0] - Vertices[LastMergeIndex]).GetSafeNormal());
		FQuat2D Initial = FQuat2D((Other.Vertices[OtherFirstMergeIndex] - Other.Vertices[Mod(OtherFirstMergeIndex + 1, Other.Vertices.Num())]).GetSafeNormal());
		FQuat2D Rotation = Initial.Inverse().Concatenate(Target);
		MergeResult.Transform = FTransform2D(Rotation, Vertices[FirstMergeIndex] - Rotation.TransformPoint(Other.Vertices[Mod(OtherLastMergeIndex + 1, Other.Vertices.Num())]));


		//Add other vertices
		OtherLastMergeIndex = Mod(OtherLastMergeIndex + 1, Other.Num());
		do
		{
			MergedShape.Vertices.Emplace(MergeResult.Transform.TransformPoint(Other.Vertices[OtherLastMergeIndex]));
			MergedShape.ShapeSockets.Emplace(OtherShapeSockets[OtherLastMergeIndex]);
			MergeResult.Growth++;

			OtherLastMergeIndex = Mod(OtherLastMergeIndex + 1, Other.Num());
		} while (OtherLastMergeIndex != OtherFirstMergeIndex);

		// /\ Merge Shapes /\ //
		

		return true;
	}

private:
	int Mod(int A, int B) const
	{
		int Value = A % B;
		if (Value < 0)
		{
			Value += B;
		}
		return Value;
	}
};