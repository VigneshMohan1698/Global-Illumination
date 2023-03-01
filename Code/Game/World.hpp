#pragma once
#include "Game/BlockIterator.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/JobSystem.hpp"
#include <vector>
#include <map>
#include <deque>

class Game;
class Chunk;

bool	operator<(const IntVec2& a, const IntVec2& b);

struct DugBlocks
{
	Vec3 m_position;
	unsigned int m_blockType;
};
class World
{
public:
	//----------------------MAIN FUNCTIONS-----------------
	World(Game* game);
	~World();
	void					Update(float deltaSeconds);
	void					Render() const;


	//---------------------CHUNK FUNCTIONS------------------
	void					ChunkActivationAndDeactivation();
	void					CreateOneChunkAndActivate(IntVec2 worldPos);
	void					ActivateChunk(IntVec2 chunkCoord);
	void					DeactivateChunk(IntVec2 chunkCoord);
	void					AddGeneratedChunkToWorld(Chunk* chunk);
	Chunk*					GetChunkAtPosition(IntVec2 worldCoord) const;
	void					BindLightPosition(float deltaSeconds);
public:
	JobSystem*					m_jobSystem = nullptr;
	float						m_lightMovementVelocity = 5.0f;
	int						    m_populatedChunks;
	int							m_currentAccelerationStructureIndex = 0;
	int							m_totalChunkVerts = 0;
	std::map<IntVec2, Chunk*>	m_activeChunks = {};
	std::map<IntVec2, Chunk*>	m_queuedChunks = {};
	int							m_chunkActivationRange = 0;
	bool						m_debugActiveChunks = false;
	int							m_numChunks = 0;
	int							m_totalChunks = 4;
	Game*						m_game = nullptr;
};