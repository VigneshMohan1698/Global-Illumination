#pragma once	
#include "Game/Chunk.hpp"
#include "Engine/Math/AABB3.hpp"

class BlockIterator
{
public:
	BlockIterator();
	BlockIterator(Chunk* chunk, int blockIndex);

	Chunk* m_chunk;
	int m_blockIndex;

public:
	Block* GetBlock();
	Vec3 GetWorldCenter();
	bool		  GetIsSolid();
	bool		  GetIsOpaque();
	AABB3		  GetBlockBounds();
	BlockIterator GetEastNeighbour();
	BlockIterator GetNorthNeighbour();
	BlockIterator GetWestNeighbour();
	BlockIterator GetSouthNeighbour();
	BlockIterator GetUpNeighbour();
	BlockIterator GetDownNeighbour();

};