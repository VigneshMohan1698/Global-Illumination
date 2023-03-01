#pragma once
#include <string>
#include <Engine/Core/SpriteAnimDefinition.hpp>
#include "Engine/Math/EulerAngles.hpp"
class BlockDefinition
{
protected:

public:
	bool		m_isSolid = false;
	bool		m_isOpaque = false;
	bool		m_isVisible = false;
	int			m_lightAmount = 0;

	std::string m_name = "";
	AABB2		m_floorWallUV;
	AABB2		m_sideWallUV;
	AABB2		m_topWallUV;
	bool		m_isTorch = false;
	bool		m_isSpecular = false;
	AABB2		m_torchUvs;
	static constexpr  int			MAX_LIGHT_INFLUENCE = 15;
	float       m_torchAngle = 35.0f;
public:
	bool DoesEmitLight() const;
	bool IsWater() const;
	bool IsLava() const;
	bool IsTorch() const;
	bool IsSpecular() const;
	EulerAngles GetTorchAngles() const;
	bool LoadFromXmlElement(const XmlElement& element);
	static void InitializeDefinitions();
	static const BlockDefinition* GetByName(const std::string& name);
	static const BlockDefinition* GetByIndex(unsigned char index);
	static const int GetIndexBasedOnName(const std::string& name);
	static std::vector<BlockDefinition*> s_BlockDefinitions;
};

