#include "Game/BlockIterator.hpp"
#include "Game/Chunk.hpp"

BlockIterator::BlockIterator()
{
}

BlockIterator::BlockIterator(Chunk* chunk, int blockIndex)
{
    m_chunk = chunk;
    m_blockIndex = blockIndex;
}

Block* BlockIterator::GetBlock()
{
    if (m_chunk)
    {
        return m_chunk->m_blocks[m_blockIndex];
    }
    return nullptr;
}

Vec3 BlockIterator::GetWorldCenter()
{
    IntVec3 blockWorld = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
    return Vec3(blockWorld.x + 0.5f, blockWorld.y + 0.5f, blockWorld.z + 0.5f);
}

bool BlockIterator::GetIsSolid()
{
    if (m_chunk == nullptr)
    {
        return false;
    }
    else if (m_blockIndex < 0 || m_blockIndex > CHUNK_BLOCKS_TOTAL - 1)
    {
        return false;
    }
    else if (m_chunk->m_blocks[m_blockIndex] == nullptr)
    {
        return false;
    }
    return BlockDefinition::GetByIndex(m_chunk->m_blocks[m_blockIndex]->m_blockType)->m_isSolid ;
}
bool BlockIterator::GetIsOpaque()
{
    return BlockDefinition::GetByIndex(m_chunk->m_blocks[m_blockIndex]->m_blockType)->m_isOpaque;
}

AABB3 BlockIterator::GetBlockBounds()
{
    Vec3 blockPosition = m_chunk->GetWorldCoordsForIndex(m_blockIndex);
    return AABB3(blockPosition, blockPosition + 1.0f);
}

//------TO DO: IMPROVE THIS FOR FASTER RAYCASTS------------------

BlockIterator BlockIterator::GetEastNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator eastBlock = m_chunk->GetEastBlock(blockCoords);
        return eastBlock;
    }
    return BlockIterator();

}
    

BlockIterator BlockIterator::GetNorthNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator northBlock = m_chunk->GetNorthBlock(blockCoords);
        return northBlock;
    }
     return BlockIterator();
}

BlockIterator BlockIterator::GetWestNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator westBlock = m_chunk->GetWestBlock(blockCoords);
        return westBlock;
    }
     return BlockIterator();
}

BlockIterator BlockIterator::GetSouthNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator southBlock = m_chunk->GetSouthBlock(blockCoords);
        return southBlock;
    }
    return BlockIterator();
}

BlockIterator BlockIterator::GetUpNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator northBlock = m_chunk->GetUpBlock(blockCoords);
        return northBlock;
    }
    return BlockIterator();
   
}

BlockIterator BlockIterator::GetDownNeighbour()
{
    if (m_chunk)
    {
        IntVec3 blockCoords = m_chunk->GetLocalCoordsforIndex(m_blockIndex);
        BlockIterator downBlock = m_chunk->GetDownBlock(blockCoords);
        return downBlock;
    }
    return BlockIterator();
}
