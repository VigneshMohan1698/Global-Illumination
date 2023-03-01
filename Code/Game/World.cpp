#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/Chunk.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRenderer.cpp"
#include "Engine/Core/JobWorker.hpp"
#include "Engine/Renderer/RendererD12.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"

extern InputSystem* g_theInputSystem;
extern RendererD12* g_theRenderer;

World::World(Game* game)
{
	m_game = game;
	m_chunkActivationRange = g_gameConfigBlackboard.GetValue("ChunkActivationRange", 0);
	m_debugActiveChunks = g_gameConfigBlackboard.GetValue("DebugActiveChunks", false);
	JobSystemConfig config;
	config.m_numWorkerThreads = std::thread::hardware_concurrency();
	m_jobSystem = new JobSystem(config);
	m_jobSystem->Startup();
}

World::~World()
{
	std::map<IntVec2, Chunk*>::iterator chunkIterator;
	for (chunkIterator = m_activeChunks.begin(); chunkIterator != m_activeChunks.end(); chunkIterator++)
	{
		delete chunkIterator->second;
	}
	m_jobSystem->ShutDown();
}
void World::Update(float deltaSeconds)
{
	//if (m_activeChunks.size() < g_theRenderer->MINECRAFTCHUNKS)
	//{
	//	
	//}

	ChunkActivationAndDeactivation();

	//---------------------UPDATE ALL CHUNKS---------------------------
	std::map<IntVec2, Chunk*>::iterator chunkIterator;

	for (chunkIterator = m_activeChunks.begin(); chunkIterator != m_activeChunks.end(); chunkIterator++)
	{
		chunkIterator->second->Update(deltaSeconds);
	}

}
void World::Render() const
{
	for (const auto& chunkIterator : m_activeChunks)
	{
		chunkIterator.second->Render();
	}
}

//-------------------------------CHUNK FUNCTIONS------------------------------------------------
void World::CreateOneChunkAndActivate(IntVec2 worldPos)
{
	Chunk* chunk;
	chunk = new Chunk(worldPos, this);
	//AddGeneratedChunkToWorld(chunk);
}
void World::ChunkActivationAndDeactivation()
{
	//if (m_game->m_accelerationStructuresBuilt)
	//{
	//return;
	//}
	int maxChunks;
	int chunkDeactivationRange = m_chunkActivationRange + CHUNK_SIZE_X + CHUNK_SIZE_Y;
	int maxChunksRadiusX = 1 + int(m_chunkActivationRange) / CHUNK_SIZE_X;
	int maxChunksRadiusY = 1 + int(m_chunkActivationRange) / CHUNK_SIZE_Y;
	if (m_debugActiveChunks)
	{
		maxChunks = 40;
		m_chunkActivationRange = 20;
	}
	else
	{
		maxChunks = (2 * maxChunksRadiusX) * (2 * maxChunksRadiusY);
	}
	bool chunkActivation = false, chunkDeactivation = false;

	Vec3 playerPositions = m_game->m_player->m_position;
	IntVec2 playerChunk = Chunk::GetChunkCoordsForWorldPosition(playerPositions);
	IntVec2 playerPosition = Vec2(playerPositions);
	IntVec2 closestProbableChunk = IntVec2::ZERO;
	IntVec2 farthestProbableChunk = IntVec2::ZERO;

	float distanceToChunk = 0.0f;
	float loopDistance = 0.0f;
	bool loopActivated = false;
	if (m_numChunks < maxChunks)
	{
		for (int x = playerChunk.x - maxChunksRadiusX; x <= playerChunk.x + maxChunksRadiusX; x++)
		{
			for (int y = playerChunk.y - maxChunksRadiusY; y <= playerChunk.y + maxChunksRadiusY; y++)
			{
				loopDistance = Vec2((float)(playerChunk.x * CHUNK_SIZE_X - x * CHUNK_SIZE_X), (float)(playerChunk.y * CHUNK_SIZE_Y - y * CHUNK_SIZE_Y)).GetLengthSquared();
				if (loopDistance < m_chunkActivationRange * chunkDeactivationRange && (loopDistance <= distanceToChunk || (distanceToChunk == 0.0f && !loopActivated))
					&& GetChunkAtPosition(IntVec2(x, y)) == nullptr)
				{
					closestProbableChunk = IntVec2(x, y);
					chunkActivation = true;
					distanceToChunk = loopDistance;
					loopActivated = true;
				}
			}
		}
	}
	if (chunkActivation)
	{
		ActivateChunk(closestProbableChunk);
	}
	Chunk* chunk = nullptr;
	std::map<IntVec2, Chunk*>::iterator chunkIterator;
	for (chunkIterator = m_activeChunks.begin(); chunkIterator != m_activeChunks.end(); chunkIterator++)
	{
		chunk = chunkIterator->second;
		if (chunk)
		{
			loopDistance = Vec2((playerPosition - chunk->GetChunkCenter())).GetLengthSquared();
			if (loopDistance > chunkDeactivationRange * chunkDeactivationRange && (loopDistance > distanceToChunk || distanceToChunk == 0.0f))
			{
				farthestProbableChunk = chunk->m_chunkCoords;
				chunkDeactivation = true;
				distanceToChunk = loopDistance;
			}
		}
	}
	if (chunkDeactivation)
	{
		DeactivateChunk(farthestProbableChunk);
	}

	//-----------------------RETRIEVE AN BUILT CHUNK FROM JOB AND ADD IT TO WORLD---------------------------
	ChunkGenerateJob* completedChunkJob = dynamic_cast<ChunkGenerateJob*>(m_jobSystem->RetrieveCompletedJobs());
	if(completedChunkJob != nullptr)
	{
		AddGeneratedChunkToWorld(completedChunkJob->m_chunk);
	}
}
void World::ActivateChunk(IntVec2 chunkCoord)
{
	//Chunk* chunk;
	//chunk = new Chunk(chunkCoord, this, (int)m_activeChunks.size());
	//AddGeneratedChunkToWorld(chunk);
	/*m_game->m_rebuildAccelerationStructures = true;*/


	//-----------------------DON'T ADD CHUNK TO JOB GENERATOR IF CHUNK BLAS IS FILLED-----------------
	if (m_game->m_canRender)
	{
		return;
	}
	if (m_queuedChunks.find(chunkCoord) == m_queuedChunks.end() && m_activeChunks.find(chunkCoord) == m_activeChunks.end())
	{
		Chunk* chunk;
		chunk = new Chunk(chunkCoord, this);
		if (chunk->m_loadedFromSaves)
		{
			AddGeneratedChunkToWorld(chunk);
		}
		else
		{
			m_queuedChunks.insert({ chunkCoord , chunk });
		}
	}
}
void World::DeactivateChunk(IntVec2 chunkCoord)
{
	Chunk* chunkToDeactivate = nullptr;
	chunkToDeactivate = GetChunkAtPosition(chunkCoord);
	if (chunkToDeactivate)
	{
		if (chunkToDeactivate->m_northChunk)
		{
			chunkToDeactivate->m_northChunk->m_southChunk = nullptr;
		}
		if (chunkToDeactivate->m_southChunk)
		{
			chunkToDeactivate->m_southChunk->m_northChunk = nullptr;
		}
		if (chunkToDeactivate->m_westChunk)
		{
			chunkToDeactivate->m_westChunk->m_eastChunk = nullptr;
		}
		if (chunkToDeactivate->m_eastChunk)
		{
			chunkToDeactivate->m_eastChunk->m_westChunk = nullptr;
		}
		chunkToDeactivate->m_northChunk = nullptr;
		chunkToDeactivate->m_southChunk = nullptr;
		chunkToDeactivate->m_eastChunk = nullptr;
		chunkToDeactivate->m_westChunk = nullptr;

		m_totalChunkVerts -= (int)chunkToDeactivate->m_totalChunkVerts;
		std::map<IntVec2, Chunk*>::iterator chunkIterator;
		chunkIterator = m_activeChunks.find(chunkCoord);
		m_activeChunks.erase(chunkIterator);

		if (chunkToDeactivate->m_needsSaving)
		{
			chunkToDeactivate->SaveToFile();
		}

		delete chunkToDeactivate;
		m_numChunks--;
	}

}
void World::AddGeneratedChunkToWorld(Chunk* chunk)
{
	if (!chunk->m_loadedFromSaves)
	{
		std::map<IntVec2, Chunk*>::iterator chunkIterator;
		chunkIterator = m_queuedChunks.find(chunk->m_chunkCoords);
		m_queuedChunks.erase(chunkIterator);
	}

	IntVec2 chunkCoord = chunk->m_chunkCoords;
	chunk->m_northChunk = GetChunkAtPosition(IntVec2(chunkCoord.x + 1, chunkCoord.y));
	chunk->m_southChunk = GetChunkAtPosition(IntVec2(chunkCoord.x - 1, chunkCoord.y));
	chunk->m_eastChunk = GetChunkAtPosition(IntVec2(chunkCoord.x, chunkCoord.y - 1));
	chunk->m_westChunk = GetChunkAtPosition(IntVec2(chunkCoord.x, chunkCoord.y + 1));

	if (chunk->m_northChunk)
	{
		chunk->m_northChunk->m_southChunk = chunk;
	}
	if (chunk->m_southChunk)
	{
		chunk->m_southChunk->m_northChunk = chunk;
	}
	if (chunk->m_eastChunk)
	{
		chunk->m_eastChunk->m_westChunk = chunk;
	}
	if (chunk->m_westChunk)
	{
		chunk->m_westChunk->m_eastChunk = chunk;
	}
	m_activeChunks.insert({ chunkCoord , chunk });
	chunk->m_chunkState = (int)ChunkStates::ACTIVE;
	m_totalChunkVerts += chunk->m_totalChunkVerts;
	chunk->m_accelerationStructureIndex = m_numChunks;
	chunk->SetInitialLightingData();
	m_numChunks++;

}
Chunk* World::GetChunkAtPosition(IntVec2 worldCoord) const
{
	if (m_activeChunks.find(worldCoord) != m_activeChunks.end())
	{
		return m_activeChunks.find(worldCoord)->second;
	}
	else return nullptr;
}

//---------------------------LIGHT-----------------------------------

bool operator<(const IntVec2& a, const IntVec2& b)
{
	if (a.y < b.y)
	{
		return true;
	}
	else if (b.y < a.y)
	{
		return false;
	}
	else return a.x < b.x;
}