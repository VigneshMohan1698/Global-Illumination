#include "BlockTemplateDefinition.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Renderer/Renderer.hpp"
extern Renderer* g_theRenderer;

std::vector<BlockTemplateDefinition*> BlockTemplateDefinition::s_BlockTemplateDefinitions = {};



bool BlockTemplateDefinition::LoadFromXmlElement(const XmlElement& element)
 {
	std::string defaultValue = "";
	std::string blockName = ParseXmlAttribute(element, "name", defaultValue);
	m_name = blockName;
	
	for (const tinyxml2::XMLElement* elements = element.FirstChildElement(); elements; elements = elements->NextSiblingElement())
	{
  		std::string blockType = ParseXmlAttribute(*elements, "type", defaultValue);
		unsigned int blockIndex = BlockDefinition::GetIndexBasedOnName(blockType);
		m_blockTypes.push_back(blockIndex);

		Vec3 offset = ParseXmlAttribute(*elements, "offset", Vec3());
 		IntVec3 offsetInt = IntVec3((int)offset.x, (int)offset.y, (int)offset.z);
		m_offsets.push_back(offsetInt);
	}
	/*std::string blockType1 = ParseXmlAttribute(element, "BlockName1", defaultValue);
	m_blockType1 = BlockDefinition::GetIndexBasedOnName(blockType1);

	std::string blockType2 = ParseXmlAttribute(element, "BlockName2", defaultValue);
	m_blockType2 = BlockDefinition::GetIndexBasedOnName(blockType2);

	m_offset1 = ParseXmlAttribute(element, "Offset1", Vec3());
	m_offset1 = ParseXmlAttribute(element, "Offset2", Vec3());*/
	return false;
}

void BlockTemplateDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument xml_doc;
	tinyxml2::XMLError result = xml_doc.LoadFile("Data/Definitions/BlockTemplateDefinitions.xml");
	if (result != tinyxml2::XML_SUCCESS)
		return;

	tinyxml2::XMLElement* rootElement = xml_doc.RootElement();
	if (rootElement != nullptr)
	{
		for (const tinyxml2::XMLElement* elements = rootElement->FirstChildElement(); elements; elements = elements->NextSiblingElement())
		{
			BlockTemplateDefinition* block = new BlockTemplateDefinition();
			block->LoadFromXmlElement(*elements);
			s_BlockTemplateDefinitions.push_back(block);
		}
	}
 	return;
}

const BlockTemplateDefinition* BlockTemplateDefinition::GetByName(const std::string& name)
{
	for (int i = 0; i < (int)s_BlockTemplateDefinitions.size(); i++)
	{
		if (s_BlockTemplateDefinitions[i]->m_name == name)
		{
			return s_BlockTemplateDefinitions[i];
		}
	}
	return nullptr;
}

const BlockTemplateDefinition* BlockTemplateDefinition::GetByIndex(unsigned char index)
{
	if (index > (int)s_BlockTemplateDefinitions.size() || index == -1)
	{
		return nullptr;
	}
	else return s_BlockTemplateDefinitions[index];
}

const int BlockTemplateDefinition::GetIndexBasedOnName(const std::string& name)
{
	for (int i = 0; i < (int)s_BlockTemplateDefinitions.size(); i++)
	{
		if (s_BlockTemplateDefinitions[i]->m_name == name)
		{
			return i;
		}
	}
	return -1;
}
