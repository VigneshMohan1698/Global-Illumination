#pragma once
#include <string>
#include "Engine/Core/SpriteAnimDefinition.hpp"
#include <Engine/Math/IntVec3.hpp>

class BlockTemplateDefinition
{
protected:

public:
	std::string  m_name = "";
	//unsigned int m_blockType1 = 0;
	//unsigned int m_blockType2 = 0;

	std::vector<unsigned int>	 m_blockTypes;
	std::vector<IntVec3>		 m_offsets;
public:
	bool LoadFromXmlElement(const XmlElement& element);
	static void InitializeDefinitions();
	static const BlockTemplateDefinition* GetByName(const std::string& name);
	static const BlockTemplateDefinition* GetByIndex(unsigned char index);
	static const int GetIndexBasedOnName(const std::string& name);
	static std::vector<BlockTemplateDefinition*> s_BlockTemplateDefinitions;
};

