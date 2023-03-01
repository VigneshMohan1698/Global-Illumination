#pragma once
#include "Game/Block.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/JobWorker.hpp"
#include <atomic>
#include "Engine/Core/Vertex_PNCU.hpp"
#include <vector>
#include "Engine/Renderer/RendererD12.hpp"

struct ChunkAccelerationStructureData
{
	GpuBuffer indexBuffer;
	GpuBuffer vertexBuffer;
	ComPtr<ID3D12Resource> bottomLevelAS;
	ComPtr<ID3D12Resource> topLevelAS;
};

enum class ChunkStates
{
	MISSING,						//MISSING DUE TO WHATEVER REASON
	ON_DISK,						//MISSING BUT AVAIALABLE ON DISK
	CONSTRUCTING,					//CONSTRUCTING

	ACTIVATING_QUEUED,				//CHUNK QUEUED FOR m_blocks CALCULATIONS
	ACTIVATING_PICKED,				//CHUNK PICKED UP FOR m_block GENERATION
	ACTIVATING_COMPLETED,			//CHUNK COMPLETED  m_block GENERATION
	ACTIVE,							//CHUNK IS ACTIVE - CAN CALL UPDATE AND RENDER ON IT

	DECONSTRUCTING,
	NUM_CHUNK_STATES
};
class World;
class BlockIterator;
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 7;

constexpr int CHUNK_SIZE_X = 1<< CHUNK_BITS_X;
constexpr int CHUNK_SIZE_Y = 1<< CHUNK_BITS_Y;
constexpr int CHUNK_SIZE_Z = 1<< CHUNK_BITS_Z;

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;


constexpr int CHUNK_BLOCKS_TOTAL  = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
constexpr int SEA_LEVEL = 50;

class ChunkGenerateJob : public Job
{
friend class Chunk;
public:
	ChunkGenerateJob(Chunk* chunk);
private:
	virtual void Execute() override;
	virtual void OnFinished() override;
public:
	Chunk* m_chunk = nullptr;
};


class Chunk
{
public:
	Chunk(IntVec2 chunkCoords, World* world);	
	~Chunk();
	World*						m_world = nullptr;
	IntVec2						m_chunkCoords = IntVec2::ZERO;
	Vec2						m_chunkPosition = Vec2::ZERO;
	int							m_chunkASInstance = -1;
	std::vector<Vertex_PNCUTB>	m_vertexes;
	std::vector<unsigned int>	m_indexes;
	bool						m_isCpuMeshDirty = false;
	//VertexBuffer*				m_vertexBuffer = nullptr;
	//IndexBuffer*				m_indexBuffer = nullptr;
	int							m_totalChunkVerts = 0;
	bool						m_needsSaving = false;
	float						m_perlinNoiseMultiplier = 30.0f;
	unsigned int				m_worldSeed = 0;
	bool						m_loadedFromSaves = false;
public:
	void						GenerateBlocks();
	void						CreateInitialChunkBlocks();
	bool						CalculateIfBlockIsTree(int x, int y, float currentGlobalX, float currentGlobalY /*IntVec2& out_treePosition, IntVec2& out_treeChunkCoord*/);
	Vec4						CalculateFaceColorFromBlock(Block* block,const BlockDefinition* currentBlockDefinition, Vec4 blockPosition = Vec4(0.0f,0.0f,0.0f,0.0f));
	void						CreateBuffers();
	void						Update(float deltaSeconds);
	void						Render() const;
	void						DebugRender() const;
	bool						PopulateChunkVertices();
	int							GetIndexForLocalCoords(IntVec3 localCoords);
	IntVec3						GetLocalCoordsforIndex(int i);
	Vec3						GetWorldCoordsForIndex(int i);
	static IntVec2				GetChunkCoordsForWorldPosition(Vec3 position);
	IntVec2						GetChunkCenter();
	void						PlaceBlock(int iteratorBlockIndex);
	void						DigBlock(int iteratorBlockIndex);
	void						SetInitialLightingData();

	bool						LoadFromFile(IntVec2 chunkCoords);
	bool						SaveToFile();
	int							GetZNonAirHeightForChunkColumn(int columnX, int columnY);

	BlockIterator				GetNorthBlock(IntVec3 blockCoords);
	BlockIterator				GetSouthBlock(IntVec3 blockCoords);
	BlockIterator				GetEastBlock(IntVec3 blockCoords);
	BlockIterator				GetWestBlock(IntVec3 blockCoords);
	BlockIterator				GetDownBlock(IntVec3 blockCoords);
	BlockIterator				GetUpBlock(IntVec3 blockCoords);

	void						BuildAccelerationStruturesData();
public:
	//const static int TOTALBLOCKS  = 32768;
	ChunkAccelerationStructureData m_chunkASData;
	int							m_accelerationStructureIndex = 0;
	AABB3						m_worldBounds;	
	Block*						m_blocks[CHUNK_BLOCKS_TOTAL];
	Chunk*						m_northChunk = nullptr;
	Chunk*						m_southChunk = nullptr;
	Chunk*						m_eastChunk = nullptr;
	Chunk*						m_westChunk = nullptr;
	std::atomic<int>			m_chunkState =  (int)ChunkStates::CONSTRUCTING;
	const static  uint8_t 		BLOCK_BIT_IS_SKY = 1;
	const static  uint8_t 		BLOCK_BIT_IS_DIRTY = 2;
	const static  uint8_t 		BLOCK_BIT_TORCH_FRONT_WALL = 4;
	const static  uint8_t 		BLOCK_BIT_TORCH_BACK_WALL = 8;
	const static  uint8_t 		BLOCK_BIT_TORCH_LEFT_WALL = 16;
	const static  uint8_t 		BLOCK_BIT_TORCH_RIGHT_WALL = 32;
	const static  float			TORCH_ANGLE;
};

