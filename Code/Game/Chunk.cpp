#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Math/VertexUtils.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/RendererD12.hpp"
#include "Game/Chunk.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/Game.hpp"
#include "Game/BlockTemplateDefinition.hpp"
#include "Game/BlockIterator.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"


extern RendererD12* g_theRenderer;
extern InputSystem* g_theInputSystem;

const float Chunk::TORCH_ANGLE = 45.0f;
Chunk::Chunk(IntVec2 chunkCoords, World* world)
{
	m_chunkCoords = chunkCoords;
	m_chunkPosition = Vec2((float)CHUNK_SIZE_X * chunkCoords.x, (float)CHUNK_SIZE_Y * chunkCoords.y);
	m_world = world;
	//m_vertexes.reserve(3000);
	//m_indexes.reserve(3000);
	Vec3 chunkMins = Vec3(0.0f, 0.0f, 0.0f);
	Vec3 chunkMaxs = Vec3(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
	Vec3 mins = Vec3(float(CHUNK_SIZE_X * m_chunkCoords.x), float(CHUNK_SIZE_Y * m_chunkCoords.y), 0.f);
	m_worldBounds = AABB3(mins, mins + Vec3(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z));
	m_isCpuMeshDirty = true;
	m_needsSaving = false;
	m_chunkState = (int)ChunkStates::CONSTRUCTING;
	GenerateBlocks();
}
void Chunk::GenerateBlocks()
{
	//if (LoadFromFile(m_chunkCoords) == false)
	//{
	//	m_worldSeed = g_gameConfigBlackboard.GetValue("WorldSeed", 0);
	//	CreateInitialChunkBlocks();
	//}
	//else
	//{
	//	m_loadedFromSaves = true;
	//}

	if (LoadFromFile(m_chunkCoords) == false)
	{
		m_worldSeed = g_gameConfigBlackboard.GetValue("WorldSeed", 0);
		ChunkGenerateJob* job = new ChunkGenerateJob(this);
		m_world->m_jobSystem->QueueJob(job);
	}
	else
	{
		m_loadedFromSaves = true;
	}
}
Chunk::~Chunk()
{
	m_vertexes.clear();
	m_indexes.clear();
	for (int i = 0; i < CHUNK_BLOCKS_TOTAL; i++)
	{
		delete m_blocks[i];
	}
}
void Chunk::DebugRender() const
{

}
void Chunk::Update(float deltaSeconds)
{
	UNUSED((void)deltaSeconds);
	if (m_isCpuMeshDirty)
	{
		PopulateChunkVertices();
		//if(populated)
		//{
		//	//m_world->m_game->m_rebuildAccelerationStructures = true;
		//	g_theRenderer->BuildBLAS(m_vertexes, m_indexes, m_accelerationStructureIndex);
		//}
	}
}
void Chunk::Render() const
{
	if (m_vertexes.empty())
	{
		return;
	}

}

//-------------------CREATE BLOCKS AND VERTICES-----------------
void Chunk::CreateInitialChunkBlocks()
{
	//----------------------GET ALL THE BLOCKS---------------------------
	const BlockDefinition* air = BlockDefinition::GetByName("Air");
	const BlockDefinition* grass = BlockDefinition::GetByName("Grass");
	const BlockDefinition* dirt = BlockDefinition::GetByName("Dirt");
	const BlockDefinition* stone = BlockDefinition::GetByName("Stone");
	const BlockDefinition* water = BlockDefinition::GetByName("Water");
	const BlockDefinition* gold = BlockDefinition::GetByName("Gold");
	const BlockDefinition* diamond = BlockDefinition::GetByName("Diamond");
	const BlockDefinition* coal = BlockDefinition::GetByName("Coal");
	const BlockDefinition* iron = BlockDefinition::GetByName("Iron");
	const BlockDefinition* sand = BlockDefinition::GetByName("Sand");
	const BlockDefinition* ice = BlockDefinition::GetByName("Ice");
	const BlockDefinition* leaves = BlockDefinition::GetByName("Leaves");
	const BlockDefinition* oaklog = BlockDefinition::GetByName("OakLog");
	//-----------------------------------------------CHECK IF ALL 4 BOUNDARY CHUNKS ARE PRESENT - IF NOT EXIT---------------------------
	RandomNumberGenerator rng;
	int terrainHeightZ = 0, blockIndex = 0;
	const BlockDefinition* blockDefinition = air;
	//----------------------------------FILLING M_BLOCKS-----------------------------------
	for (int y = 0; y < CHUNK_SIZE_Y; y++)
	{
		for (int x = 0; x < CHUNK_SIZE_X; x++)
		{
			float globalX, globalY;
			globalX = (float)(m_chunkCoords.x * CHUNK_SIZE_X) + x;
			globalY = (float)(m_chunkCoords.y * CHUNK_SIZE_Y) + y;
			float humidityNoise = Compute2dPerlinNoise(float(globalX), float(globalY), 500, 8, 0.5f, 2.0f, 0, m_worldSeed + 1);
			float temperatureNoise = Compute2dPerlinNoise(float(globalX), float(globalY), 200.f, 5, 0.5f, 2.0f, 0, m_worldSeed + 2);
			float hillinessNoise = Compute2dPerlinNoise(float(globalX), float(globalY), 100.0f, 4, 0.5f, 2.0f, 0, m_worldSeed + 3);
			float oceanNoise = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 400.0f, 5, 0.5f, 2.0f, 0, m_worldSeed + 3);
			hillinessNoise = RangeMapZeroToOne(hillinessNoise, -1.0f, 1.0f);
			//humidityNoise = SmoothStart2(humidityNoise);
		 //   temperatureNoise = SmoothStep3(temperatureNoise);
			//hillinessNoise = SmoothStep3(hillinessNoise);

			float terrainHeightNoise = Compute2dPerlinNoise(float(globalX), float(globalY), 800.f, 15, 0.5f, 2.0f, 0, m_worldSeed);
			terrainHeightNoise = SmoothStep3(fabsf(terrainHeightNoise) * hillinessNoise);
			terrainHeightNoise = RangeMap(terrainHeightNoise, 0.0f, 1.0f, SEA_LEVEL - 10, SEA_LEVEL + 60);
			terrainHeightZ = (int)terrainHeightNoise;

			//------------------OCEAN--------------------
			//if (oceanNoise > 0.5f)
			//{
			//	terrainHeightZ = SEA_LEVEL;
			//}
	/*		if (oceanNoise > 0.0f && oceanNoise < 0.5f)
			{
				float oceanRangeMap = RangeMap(oceanNoise, 0.0f,0.5f, 1.0f,0.0f);
				float terrainVsOcean = Interpolate((float)terrainHeightZ, (float)SEA_LEVEL, oceanRangeMap);
				terrainHeightZ = (int)terrainVsOcean;
			}*/
			//terrainHeightZ = CHUNK_SIZE_Z / 2 + int(30.f * Compute2dPerlinNoise(float(globalX), float(globalY), 200.f, 5, 0.5f, 2.0f, 0));
			//---------------------------FILLING Z COLUMN BLOCKS WITH WORLD GENERATION---------------------------
			for (int z = 0; z < CHUNK_SIZE_Z; z++)
			{
				blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z));
				if (m_blocks[blockIndex] != nullptr)
				{
					continue;
				}
				//----------------------------TIER 1 WORLD GENERATION----------------------------------
				if (z > terrainHeightZ && z < SEA_LEVEL)
				{
					blockDefinition = grass;
				}
				else if (z == terrainHeightZ)
				{
					blockDefinition = grass;
				}
				else if (z >= terrainHeightZ - 4 && z <= terrainHeightZ - 1)
				{
					blockDefinition = dirt;
				}
				else if (z < terrainHeightZ - 4)
				{
					blockDefinition = stone;
				}
				else
				{
					blockDefinition = BlockDefinition::GetByName("Air");
				}
				if (z >= 0 && z < SEA_LEVEL - 6 && z < terrainHeightZ)
				{
					if (rng.GetRandomIntInRange(1, 1000) == 1)
					{
						blockDefinition = diamond;
					}
					else if (rng.GetRandomIntInRange(1, 200) == 1)
					{
						blockDefinition = gold;
					}
					else if (rng.GetRandomIntInRange(1, 50) == 1)
					{
						blockDefinition = iron;
					}
					else if (rng.GetRandomIntInRange(1, 20) == 1)
					{
						blockDefinition = coal;
					}
				}
				if (z < SEA_LEVEL && z > SEA_LEVEL - 5)
				{
					blockDefinition = water;
				}

				//------------------SAND-----------------------------
				if (humidityNoise < 0.4f)
				{
					int numSandBlocks = (int)RangeMap(humidityNoise, 0.0f, 0.4f, 0.0f, (terrainHeightZ - (float)SEA_LEVEL - 1));
					if (z <= terrainHeightZ && z > terrainHeightZ - numSandBlocks && blockDefinition != water)
					{
						blockDefinition = sand;
					}
				}
				//------------------ICE---------------------
				if (temperatureNoise < 0.4f)
				{
					int numIceBlocks = (int)RangeMap(humidityNoise, 0.0f, 0.4f, 0.0f, 4.0f);
					if (z <= terrainHeightZ && z > terrainHeightZ - numIceBlocks && blockDefinition == water)
					{
						blockDefinition = ice;
					}
				}
				//-----------------BEACHES-----------------
				if (humidityNoise < 0.6f && z == SEA_LEVEL && blockDefinition == grass)
				{
					blockDefinition = sand;
				}

				//----------------TREES----------------------------
				const BlockTemplateDefinition* blockTemplate = BlockTemplateDefinition::GetByName("BirchTree");
				if (z == terrainHeightZ && z >= SEA_LEVEL && blockDefinition != water)
				{
					bool isBlockTree = CalculateIfBlockIsTree(x, y, globalX, globalY);

					if (isBlockTree)
					{
						for (int i = 0; i < blockTemplate->m_blockTypes.size(); i++)
						{
							IntVec3 offset = blockTemplate->m_offsets[i];
							offset = offset + IntVec3(x, y, z);
							if (offset.x < CHUNK_SIZE_X && offset.x >= 0 && offset.y < CHUNK_SIZE_Y && offset.y >= 0)
							{
								unsigned int blockTypeIndex = blockTemplate->m_blockTypes[i];
								blockIndex = GetIndexForLocalCoords(offset);
								Block* block = new Block((uint8_t)blockTypeIndex);
								m_blocks[blockIndex] = block;
								continue;
							}
						}

					}

				}

				blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z));
				Block* block = new Block((unsigned char)BlockDefinition::GetIndexBasedOnName(blockDefinition->m_name));
				m_blocks[blockIndex] = block;
			}

		}
	}
}
bool Chunk::PopulateChunkVertices()
{
	//--------------------------------------------CHECK IF NEIGHBOURS ARE PRESENT-------------------------------------
	if (m_westChunk == nullptr || m_eastChunk == nullptr || m_northChunk == nullptr || m_southChunk == nullptr)
	{
		m_isCpuMeshDirty = true;
		return false;
	}
	/*float time = GetCurrentTimeSeconds();*/
	std::vector<Vertex_PNCUTB>		m_torchVertices;
	std::vector<unsigned int>	    m_torchIndices;
	int torchIndexCount = 0;

	m_vertexes.clear();
	m_indexes.clear();
	unsigned int m_indexCount = 0;
	int blockIndex = 0;
	Texture* chunkTexture = nullptr;
	IntVec2 dimensions = IntVec2(2048, 2048);
	std::vector<Vertex_PNCUTB>& vertices = g_theRenderer->m_DXRverts;
	std::vector<UINT>& indexes = g_theRenderer->m_DXRindexes;
	//std::vector<Vertex_PNCUT>& vertices = m_vertexes;
	//std::vector<UINT>& indexes = m_indexes;
	SpriteSheet m_mapSpriteSheet = SpriteSheet(*chunkTexture, dimensions, IntVec2(64, 64), true);
	const BlockDefinition* blockDefinition = BlockDefinition::GetByName("Diamond");
	//------------------------------------------------ADDING VERTS------------------------------------------
	BlockIterator iterator;
	Vec4 facecolor;
	Block* block = nullptr;
	Vec2 UV = Vec2(33, 34);
	m_indexCount = (unsigned int)g_theRenderer->m_DXRverts.size();
	Vec3 quadNormal = Vec3();
	Vec2 uvs;
	AABB3 blockbounds = AABB3::ZERO_TO_ONE;
	for (int z = 0; z < CHUNK_SIZE_Z; z++)
	{
		for (int y = 0; y < CHUNK_SIZE_Y; y++)
		{
			Vec4 blockCenter = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
			for (int x = 0; x < CHUNK_SIZE_X; x++)
			{
				blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z));
				iterator = BlockIterator(this, blockIndex);
				blockDefinition = BlockDefinition::GetByIndex(m_blocks[blockIndex]->m_blockType);
				if (!blockDefinition->m_isVisible)
				{
					continue;
				}
				Vec3 torchTranslate = Vec3();
				blockbounds = AABB3(Vec3((float)x + m_chunkPosition.x, (float)y + m_chunkPosition.y, (float)z), Vec3(x + m_chunkPosition.x + 1.0f, y + m_chunkPosition.y + 1.0f, z + 1.0f));
				Vec3 bottomLeft, bottomRight, topLeft, topRight;
				float minx, miny, minz, maxx, maxy, maxz;

				if (blockDefinition->IsTorch())
				{
					torchTranslate = Vec3((float)x + m_chunkPosition.x + 0.45f, (float)y + m_chunkPosition.y + 0.45f, (float)z);
					blockbounds.m_mins = Vec3(0.0f, 0.0f, 0.0f);
					blockbounds.m_maxs = Vec3(0.1f, 0.1f, 0.8f);
				}


				minx = blockbounds.m_mins.x;
				miny = blockbounds.m_mins.y;
				minz = blockbounds.m_mins.z;

				maxx = blockbounds.m_maxs.x;
				maxy = blockbounds.m_maxs.y;
				maxz = blockbounds.m_maxs.z;

				bottomLeft = Vec3(maxx, maxy, minz);
				bottomRight = Vec3(maxx, miny, minz);
				topLeft = Vec3(minx, maxy, minz);
				topRight = Vec3(minx, miny, minz);

				blockCenter = Vec4(blockbounds.GetCenter(), 1.0f);
				//------------------------------------FLOOR WALL ---------------------------------------
				block = iterator.GetDownNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					if (!(block->IsWater() && blockDefinition->IsWater()))
					{
						/*		if (block->DoesEmitLight())
								{
									facecolor =
								}*/
						uvs = Vec2(blockDefinition->m_floorWallUV.m_mins.x, blockDefinition->m_floorWallUV.m_mins.y + 1);
						quadNormal = Vec3(0.0f, 0.0f, -1.0f);
						facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
							m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs());
						m_indexCount += 4;

					}

				}

				bottomLeft = Vec3(maxx, miny, maxz);
				bottomRight = Vec3(maxx, maxy, maxz);
				topLeft = Vec3(minx, miny, maxz);
				topRight = Vec3(minx, maxy, maxz);

				//----------------------------------ROOF WALL-----------------------------------------------
				block = iterator.GetUpNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					uvs = Vec2(blockDefinition->m_topWallUV.m_mins.x, blockDefinition->m_topWallUV.m_mins.y + 1);
					quadNormal = Vec3(0.0f, 0.0f, 1.0f);
					facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
					if (blockDefinition->IsTorch())
					{
						AddVertsForIndexedPNCUQuadtangent3D(m_torchVertices, m_torchIndices, quadNormal, torchIndexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
							m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs());
						torchIndexCount += 4;
					}
					else
					{
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
							m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs());
						m_indexCount += 4;
					}
					

				}

				//----------------------LEFT WALL-------------------------------

				bottomLeft = Vec3(maxx, maxy, minz);
				bottomRight = Vec3(minx, maxy, minz);
				topLeft = Vec3(maxx, maxy, maxz);
				topRight = Vec3(minx, maxy, maxz);


				block = iterator.GetWestNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					if (!(block->IsWater() && blockDefinition->IsWater()))
					{
						uvs = Vec2(blockDefinition->m_sideWallUV.m_mins.x, blockDefinition->m_sideWallUV.m_mins.y + 1);
						AABB2 aabbUVs = m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs();
						if (blockDefinition->IsTorch())
						{
							aabbUVs = blockDefinition->m_torchUvs;
						}
						quadNormal = Vec3(0.0f, 1.0f, 0.0f);
						facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
						if (blockDefinition->IsTorch())
						{
							AddVertsForIndexedPNCUQuadtangent3D(m_torchVertices, m_torchIndices, quadNormal, torchIndexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
								aabbUVs);
							torchIndexCount += 4;
						}
						else
						{
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor, aabbUVs);
						m_indexCount += 4;
						}

					}
				}

				//-------------------------RIGHT WALL-----------------------------
				bottomLeft = Vec3(minx, miny, minz);
				bottomRight = Vec3(maxx, miny, minz);
				topLeft = Vec3(minx, miny, maxz);
				topRight = Vec3(maxx, miny, maxz);

				block = iterator.GetEastNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					if (!(block->IsWater() && blockDefinition->IsWater()))
					{
						uvs = Vec2(blockDefinition->m_sideWallUV.m_mins.x, blockDefinition->m_sideWallUV.m_mins.y + 1);
						AABB2 aabbUVs = m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs();
						if (blockDefinition->IsTorch())
						{
							aabbUVs = blockDefinition->m_torchUvs;
						}
						quadNormal = Vec3(0.0f, -1.0f, 0.0f);
						facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
						if (blockDefinition->IsTorch())
						{
							AddVertsForIndexedPNCUQuadtangent3D(m_torchVertices, m_torchIndices, quadNormal, torchIndexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
								aabbUVs);
							torchIndexCount += 4;
						}
						else
						{
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor, aabbUVs);
						m_indexCount += 4;
						}

					}
				}

				//------------------------BACK  WALL---------------------------
				bottomLeft = Vec3(minx, maxy, minz);
				bottomRight = Vec3(minx, miny, minz);
				topLeft = Vec3(minx, maxy, maxz);
				topRight = Vec3(minx, miny, maxz);

				block = iterator.GetSouthNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					if (!(block->IsWater() && blockDefinition->IsWater()))
					{
						uvs = Vec2(blockDefinition->m_sideWallUV.m_mins.x, blockDefinition->m_sideWallUV.m_mins.y + 1);
						AABB2 aabbUVs = m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs();
						if (blockDefinition->IsTorch())
						{
							aabbUVs = blockDefinition->m_torchUvs;
						}
						quadNormal = Vec3(-1.0f, 0.0f, 0.0f);
						facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
						if (blockDefinition->IsTorch())
						{
							AddVertsForIndexedPNCUQuadtangent3D(m_torchVertices, m_torchIndices, quadNormal, torchIndexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
								aabbUVs);
							torchIndexCount += 4;
						}
						else
						{
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
							aabbUVs);
						m_indexCount += 4;
						}
					}
				}
				//----------------------FRONT WALL------------------------------
				bottomLeft = Vec3(maxx, miny, minz);
				bottomRight = Vec3(maxx, maxy, minz);
				topLeft = Vec3(maxx, miny, maxz);
				topRight = Vec3(maxx, maxy, maxz);

				block = iterator.GetNorthNeighbour().GetBlock();
				if ((block != nullptr && !block->IsOpaque()) || blockDefinition->IsTorch())
				{
					if (!(block->IsWater() && blockDefinition->IsWater()))
					{
						uvs = Vec2(blockDefinition->m_sideWallUV.m_mins.x, blockDefinition->m_sideWallUV.m_mins.y + 1);
						AABB2 aabbUVs = m_mapSpriteSheet.GetSpriteDef(uvs, IntVec2(64, 64)).GetUVs();
						if (blockDefinition->IsTorch())
						{
							aabbUVs = blockDefinition->m_torchUvs;
						}
						quadNormal = Vec3(1.0f, 0.0f, 0.0f);
						facecolor = CalculateFaceColorFromBlock(block, blockDefinition);
						if (blockDefinition->IsTorch())
						{
							AddVertsForIndexedPNCUQuadtangent3D(m_torchVertices, m_torchIndices, quadNormal, torchIndexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
								aabbUVs);
							torchIndexCount += 4;
						}
						else
						{
						AddVertsForIndexedPNCUQuadtangent3D(vertices, indexes, quadNormal, m_indexCount, topLeft, bottomLeft, bottomRight, topRight, facecolor,
							aabbUVs);
						m_indexCount += 4;
						}
					}
				}
				if (blockDefinition->DoesEmitLight())
				{
					g_theRenderer->AddPointLights(blockCenter);
				}
				else if (blockDefinition->IsTorch())
				{
					g_theRenderer->AddPointLights(torchTranslate);
				}
				//------------TRANSFORMING TORCH VERTICES------------------
				if (blockDefinition->IsTorch())
				{
					EulerAngles rotation = blockDefinition->GetTorchAngles();
					Mat44 rotationMatrix = rotation.GetAsMatrix_XFwd_YLeft_ZUp();
					Mat44 translationMatrix = Mat44::CreateTranslation3D(torchTranslate);
					//rotationMatrix.Append(translationMatrix);
					TransformVertexArrayUsingMatrix3D((int)m_torchVertices.size(), m_torchVertices, rotationMatrix);
					TransformVertexArray3D((int)m_torchVertices.size(), m_torchVertices, 1.0f, 0.0f, torchTranslate);

					for (int k = 0; k < m_torchVertices.size(); k++)
					{
						vertices.push_back(m_torchVertices[k]);
					}
					for (int j = 0; j < m_torchIndices.size(); j++)
					{
						indexes.push_back(m_torchIndices[j] + m_indexCount);
					}
					m_indexCount += 20;
				}
				m_torchVertices.clear();
				m_torchIndices.clear();
			}

		}
		m_isCpuMeshDirty = false;
		m_totalChunkVerts = m_indexCount;
		m_world->m_totalChunkVerts += m_indexCount;
		/*CreateBuffers();*/
	}
	if (m_chunkASInstance == -1)
	{
		m_chunkASInstance = m_world->m_populatedChunks;
	}

	return true;
}
bool Chunk::CalculateIfBlockIsTree(int x, int y, float currentGlobalX, float currentGlobalY)
{
	IntVec2 chunkCheckingCoords = m_chunkCoords;
	float maxtreeNoise = -100000.0f;
	float finalX = 0.0f, finalY = 0.0f;
	for (int i = -5; i <= 5; i++)
	{
		for (int j = -5; j <= 5; j++)
		{
			float globalX, globalY;
			if (x + i > CHUNK_MAX_X)
			{
				chunkCheckingCoords.x += 1;
			}
			if (x + i < 0)
			{
				chunkCheckingCoords.x -= 1;
			}
			if (y + j > CHUNK_MAX_Y)
			{
				chunkCheckingCoords.y += 1;
			}
			if (y + j < 0)
			{
				chunkCheckingCoords.y -= 1;
			}

			globalX = (float)(chunkCheckingCoords.x * CHUNK_SIZE_X) + x + i;
			globalY = (float)(chunkCheckingCoords.y * CHUNK_SIZE_Y) + y + j;

			float treeDensityNoise = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 600, 5, 0.5f, 2.0f, 0, m_worldSeed + 3);
			float treeNoiseOctaves = RangeMap(treeDensityNoise, 0.0f, 1.0f, 3.0f, 6.0f);
			float treeNoise = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 100.0f, (unsigned int)treeNoiseOctaves, 3, 2.0f, 0, m_worldSeed + 3);

			if (treeNoise > maxtreeNoise)
			{
				maxtreeNoise = treeNoise;
				finalX = globalX;
				finalY = globalY;
			}

		}
	}
	int edgeX = (int)(fabs((int)currentGlobalX % (int)CHUNK_SIZE_X));
	int edgeY = (int)(fabs((int)currentGlobalY % (int)CHUNK_SIZE_Y));
	if (finalX == currentGlobalX && finalY == currentGlobalY && (edgeX != 1 && edgeY != 1 && edgeX != 15 && edgeY != 15))
	{
		return true;
	}
	return false;
}
void Chunk::CreateBuffers()
{
	size_t size = sizeof(Vertex_PCU) * m_vertexes.size();
	if (size != 0)
	{

	}
}

//-----------------------GET FUCNTIONS--------------------
int Chunk::GetIndexForLocalCoords(IntVec3 localCoords)
{
	return localCoords.x | (localCoords.y << CHUNK_BITS_X) | (localCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y));
}
IntVec3 Chunk::GetLocalCoordsforIndex(int i)
{
	int x, y, z;
	x = i & CHUNK_MAX_X;
	z = (i >> 8) & CHUNK_MAX_Z;
	y = (i >> 4) & CHUNK_MAX_X;
	return IntVec3(x, y, z);
}
Vec3 Chunk::GetWorldCoordsForIndex(int i)
{
	IntVec3 localCoords = GetLocalCoordsforIndex(i);
	Vec3 m_chunkMins = m_worldBounds.m_mins;
	return Vec3(m_chunkMins.x + localCoords.x, m_chunkMins.y + localCoords.y, m_chunkMins.z + localCoords.z);
}
IntVec2 Chunk::GetChunkCoordsForWorldPosition(Vec3 position)
{
	int x = RoundDownToInt(position.x) >> CHUNK_BITS_X;
	int y = RoundDownToInt(position.y) >> CHUNK_BITS_Y;
	return IntVec2(x, y);
}
IntVec2 Chunk::GetChunkCenter()
{
	float globalX, globalY;
	globalX = (m_chunkCoords.x * CHUNK_SIZE_X) + CHUNK_SIZE_X / 2.0f;
	globalY = (m_chunkCoords.y * CHUNK_SIZE_Y) + CHUNK_SIZE_Y / 2.0f;
	Vec2 returnValue = Vec2(globalX, globalY);
	return IntVec2(returnValue);
}
Vec4 Chunk::CalculateFaceColorFromBlock(Block* block, const  BlockDefinition* currentBlockDefinition, Vec4 blockCenter)
{
	UNUSED((void)block);
	if (currentBlockDefinition->DoesEmitLight())
	{
		return Vec4(1, 1, 0, 1);
	}
	if (currentBlockDefinition->IsLava())
	{
		return Vec4(1, 0, 0, 1);
	}
	if (currentBlockDefinition->IsWater())
	{
		return Vec4(0, 1, 0, 0);
	}
	if (currentBlockDefinition->IsTorch())
	{
		return Vec4(1.0f, 0.6f, 0.0f, 1.0f);
	}
	if (currentBlockDefinition->IsSpecular())
	{
		return Vec4(0, 0, 1, 0);
	}
	return Vec4(0, 0, 0, 0);
}

//------------------------SAVE / LOAD ------------------------
bool Chunk::LoadFromFile(IntVec2 chunkCoords)
{
	std::vector<uint8_t> outBlocks;
	std::string chunkFile = Stringf("Saves/Chunk(%i,%i).chunk", chunkCoords.x, chunkCoords.y);
	if (!FileExists(chunkFile))
	{
		return  false;
	}
	/*float time = GetCurrentTimeSeconds();*/
	FileReadToBuffer(outBlocks, chunkFile);
	if ((outBlocks[0] == 'G' && outBlocks[1] == 'C' && outBlocks[2] == 'H' && outBlocks[3] == 'K') == false)
	{
		return false;
	}
	//int worldSeed = outBlocks[8];

	//int gameWorldSeed = g_gameConfigBlackboard.GetValue("WorldSeed", -1);
	//if (worldSeed != gameWorldSeed)
	//{
	//	return false;
	//}
	std::vector<unsigned char> blockTypes;
	for (int i = 9; i < (int)outBlocks.size() - 2; i += 2)
	{
		unsigned char numberOfBlocks = outBlocks[i + 1];
		unsigned char blockIndex = outBlocks[i];
		for (int j = 0; j < numberOfBlocks; j++)
		{
			blockTypes.push_back(blockIndex);
		}
	}
	for (int i = 0; i < (int)blockTypes.size(); i++)
	{
		Block* block = new Block(blockTypes[i]);
		m_blocks[i] = block;
	}
	//float time2 = GetCurrentTimeSeconds();
	//float timeTakenToLoadFile = (time2 - time ) * 1000.0f;

	//std::string message = Stringf("Time Taken To Load File %.3f", timeTakenToLoadFile);
	/*DebugAddMessage(message, -1.0f, Rgba8::CYAN, Rgba8::CYAN);*/
	return true;
}
bool Chunk::SaveToFile()
{
	/*float time = GetCurrentTimeSeconds();*/
	std::vector<uint8_t> outBlocks;
	outBlocks.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Y);
	outBlocks.push_back('G');
	outBlocks.push_back('C');
	outBlocks.push_back('H');
	outBlocks.push_back('K');
	outBlocks.push_back(1);
	outBlocks.push_back(CHUNK_BITS_X);
	outBlocks.push_back(CHUNK_BITS_Y);
	outBlocks.push_back(CHUNK_BITS_Z);
	outBlocks.push_back((uint8_t)m_worldSeed);
	int totalBlocks = 0;
	for (int i = 0; i < CHUNK_BLOCKS_TOTAL; )
	{
		uint8_t currentBlockType = m_blocks[i]->m_blockType;
		if (m_chunkCoords == IntVec2(0, 4) || m_chunkCoords == IntVec2(0, 5) || m_chunkCoords == IntVec2(0, 6) || m_chunkCoords == IntVec2(0, 7) ||
			m_chunkCoords == IntVec2(-1, 4) || m_chunkCoords == IntVec2(-1, 5) || m_chunkCoords == IntVec2(-1, 6) || m_chunkCoords == IntVec2(-1, 7))
		{
			if (currentBlockType == 4)
			{
				currentBlockType = 20;
			}
		}

		uint8_t numberOfBlocks = 0;
		while (i + numberOfBlocks < CHUNK_BLOCKS_TOTAL && m_blocks[i + numberOfBlocks]->m_blockType == currentBlockType && numberOfBlocks < 255)
		{
			numberOfBlocks++;
		}

		outBlocks.push_back(currentBlockType);
		outBlocks.push_back(numberOfBlocks);
		i += numberOfBlocks;
		totalBlocks += numberOfBlocks;
	}

	std::string chunkFile = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
	FileWriteFromBuffer(outBlocks, chunkFile);
	m_needsSaving = false;
	/*float time2 = GetCurrentTimeSeconds();
	float timeTaken = (time2 - time) * 1000.0f;
	std::string message = Stringf("Time taken to Save chunk file %.3f", timeTaken);*/
	//DebugAddMessage(message, 10.0f, Rgba8::CYAN, Rgba8::CYAN);
	return true;
}


void Chunk::SetInitialLightingData()
{
	BlockIterator iterator;
	int blockIndex = 0;
	//const BlockDefinition* blockDefinition = nullptr;

	//-----------------------------LOOP EACH COLUMN FROM TOP AND SETSKY -------------------------------------------------
	for (int y = 0; y < CHUNK_SIZE_Y; y++)
	{
		for (int x = 0; x < CHUNK_SIZE_X; x++)
		{
			for (int z = CHUNK_MAX_Z; z >= 0; z--)
			{
				blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z));
				if (BlockDefinition::GetByIndex(m_blocks[blockIndex]->m_blockType)->m_isOpaque)
				{
					break;
				}
				m_blocks[blockIndex]->SetIsBlockSky(true);
			}
		}
	}
}
int Chunk::GetZNonAirHeightForChunkColumn(int x, int y)
{
	for (int z = 0; z < CHUNK_SIZE_Z; z++)
	{
		uint8_t blockType = m_blocks[GetIndexForLocalCoords(IntVec3(x, y, z))]->m_blockType;
		if (BlockDefinition::s_BlockDefinitions[blockType]->m_name == "Air")
		{
			if (z - 1 > CHUNK_MAX_Z)
			{
				return CHUNK_MAX_Z;
			}
			return z - 1;
		}
	}
	return CHUNK_MAX_Z;
}

BlockIterator Chunk::GetNorthBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;

	if (x + 1 == CHUNK_SIZE_X)
	{
		if (m_northChunk)
		{
			blockIndex = GetIndexForLocalCoords(IntVec3(0, y, z));
			return BlockIterator(m_northChunk, blockIndex);
		}
	}
	else
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x + 1, y, z));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}
BlockIterator Chunk::GetSouthBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;

	if (x - 1 == -1)
	{
		if (m_southChunk)
		{
			blockIndex = GetIndexForLocalCoords(IntVec3(CHUNK_MAX_X, y, z));
			return BlockIterator(m_southChunk, blockIndex);
		}
	}
	else
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x - 1, y, z));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}
BlockIterator Chunk::GetEastBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;
	if (y - 1 == -1)
	{
		if (m_eastChunk)
		{
			blockIndex = GetIndexForLocalCoords(IntVec3(x, CHUNK_MAX_Y, z));
			return BlockIterator(m_eastChunk, blockIndex);
		}
	}
	else
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x, y - 1, z));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}
BlockIterator Chunk::GetWestBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;

	if (y + 1 == CHUNK_SIZE_Y)
	{
		if (m_westChunk)
		{
			blockIndex = GetIndexForLocalCoords(IntVec3(x, 0, z));
			return BlockIterator(m_westChunk, blockIndex);
		}
	}
	else
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x, y + 1, z));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}
BlockIterator Chunk::GetDownBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;
	if (z > 0 && z <= CHUNK_MAX_Z)
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z - 1));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}
BlockIterator Chunk::GetUpBlock(IntVec3 blockCoords)
{
	int x = blockCoords.x, y = blockCoords.y, z = blockCoords.z;
	int blockIndex = -1;

	if (z >= 0 && z < CHUNK_MAX_Z)
	{
		blockIndex = GetIndexForLocalCoords(IntVec3(x, y, z + 1));
		return BlockIterator(this, blockIndex);
	}
	return BlockIterator();
}

void Chunk::BuildAccelerationStruturesData()
{
	/*if (m_chunkASInstance < 10)
	{
		g_theRenderer->BuildGeometryAndASForChunk(m_vertexes, m_indexes, m_chunkASData.topLevelAS,
			m_chunkASData.bottomLevelAS, m_chunkASData.vertexBuffer, m_chunkASData.indexBuffer, m_chunkASInstance);
	}*/
}
ChunkGenerateJob::ChunkGenerateJob(Chunk* chunk) : Job((int)ChunkStates::ACTIVATING_QUEUED)
{
	m_chunk = chunk;
}

void ChunkGenerateJob::Execute()
{
	m_chunk->m_chunkState = (int)ChunkStates::ACTIVATING_PICKED;
	m_chunk->CreateInitialChunkBlocks();
	OnFinished();
}

void ChunkGenerateJob::OnFinished()
{
	m_chunk->m_chunkState = (int)ChunkStates::ACTIVATING_COMPLETED;
}
