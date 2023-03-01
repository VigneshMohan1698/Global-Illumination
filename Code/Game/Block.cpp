#include "Block.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Chunk.hpp"

Block::Block()
{
}

Block::Block(uint8_t BlockType)
{
	m_blockType = BlockType;
	m_lightingInfluences = 0b00000000;
	m_bitflags = 0b00000000;
}

void Block::Update(float deltaSeconds)
{
	UNUSED((void)deltaSeconds);
}

void Block::Render() const
{
}

//void Block::CalculateLightInfluence()
//{
//	if (IsBlockSky())
//	{
//		SetOutdoorLight(BlockDefinition::MAX_LIGHT_COLOR);
//	}
//}
int Block::GetIndoorLight()
{
	return m_lightingInfluences & 0b00001111;
}

int Block::GetOutdoorLight()
{
	return (m_lightingInfluences & 0b11110000) >> 4;
}

void Block::SetIndoorlight(int light)
{
	m_lightingInfluences &= 0b11110000;
	m_lightingInfluences |= light;
}

void Block::SetOutdoorLight(int light)
{
	m_lightingInfluences &= 0b00001111;
	m_lightingInfluences |= (light << 4);
}
bool Block::IsBlockSky()
{
	return (m_bitflags & Chunk::BLOCK_BIT_IS_SKY) == Chunk::BLOCK_BIT_IS_SKY;
}
bool Block::GetIsLightDirty()
{
	return (m_bitflags & Chunk::BLOCK_BIT_IS_DIRTY) == Chunk::BLOCK_BIT_IS_DIRTY;
}
void Block::DirtyLight()
{
	m_bitflags |= Chunk::BLOCK_BIT_IS_DIRTY;
}
void Block::UndirtyLight()
{
	m_bitflags &= ~Chunk::BLOCK_BIT_IS_DIRTY;
}

EulerAngles Block::GetTorchAngles()
{
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	if ((m_bitflags & Chunk::BLOCK_BIT_TORCH_FRONT_WALL) == Chunk::BLOCK_BIT_TORCH_FRONT_WALL)
	{
		pitch = -Chunk::TORCH_ANGLE;
	}
	if ((m_bitflags & Chunk::BLOCK_BIT_TORCH_BACK_WALL) == Chunk::BLOCK_BIT_TORCH_BACK_WALL)
	{
		pitch = Chunk::TORCH_ANGLE;
	}
	if ((m_bitflags & Chunk::BLOCK_BIT_TORCH_LEFT_WALL) == Chunk::BLOCK_BIT_TORCH_LEFT_WALL)
	{
		yaw = -Chunk::TORCH_ANGLE;
	}
	if ((m_bitflags & Chunk::BLOCK_BIT_TORCH_RIGHT_WALL) == Chunk::BLOCK_BIT_TORCH_RIGHT_WALL)
	{
		yaw = Chunk::TORCH_ANGLE;
	}
	return EulerAngles(yaw, pitch, roll);
}
bool Block::IsWater()
{
	return m_blockType == BlockDefinition::GetIndexBasedOnName("Water");
}
bool Block::IsAir()
{
	return m_blockType == BlockDefinition::GetIndexBasedOnName("Air");
}

bool Block::IsTorch()
{
	return m_blockType == BlockDefinition::GetIndexBasedOnName("Torch");
}
void Block::SetIsBlockSky(bool isSky)
{
	if (isSky)
	{
		m_bitflags |= Chunk::BLOCK_BIT_IS_SKY;
	}
	else
	{
		m_bitflags &= ~Chunk::BLOCK_BIT_IS_SKY;
	}
}

void Block::SetTorchAngle(int torchAngle)
{
	m_bitflags |= torchAngle;
}

bool Block::DoesEmitLight()
{
	return BlockDefinition::GetByIndex(m_blockType)->DoesEmitLight();
}

bool Block::IsOpaque()
{
	const BlockDefinition* def = BlockDefinition::GetByIndex(m_blockType);
	if (def != nullptr)
	{
		return def->m_isOpaque;
	}
	else return false;
}
