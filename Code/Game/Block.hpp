#pragma once
#include "Game/BlockDefinition.hpp"
#include "Engine/Math/EulerAngles.hpp"
class Block
{
public:
	Block();
	Block(uint8_t BlockType);
public:
	uint8_t m_blockType;
	uint8_t m_lightingInfluences;
	uint8_t m_bitflags;
public:
	void Update(float deltaSeconds);
	void Render() const;

	//void CalculateLightInfluence();
	int GetIndoorLight();
	int GetOutdoorLight();
	void SetIndoorlight(int light);
	void SetOutdoorLight(int light);

	bool IsWater();
	bool IsAir();
	bool IsTorch();
	void SetIsBlockSky(bool isSky);
	void SetTorchAngle(int torchAngle);
	bool DoesEmitLight();
	bool IsOpaque();
	bool IsBlockSky();
	bool GetIsLightDirty();
	void DirtyLight();
	void UndirtyLight();
	EulerAngles GetTorchAngles();

};

