#include "BlockDefinition.hpp"
#include <Engine/Renderer/RendererD12.hpp>
extern RendererD12* g_theRenderer;

std::vector<BlockDefinition*> BlockDefinition::s_BlockDefinitions = {};

bool BlockDefinition::LoadFromXmlElement(const XmlElement& element)
{
	Vec2     floorWallUV;
	Vec2     sideWallUV;
	Vec2     topWallUV;

	std::string defaultValue = "";
	std::string blockName = ParseXmlAttribute(element, "name", defaultValue);
	m_name = blockName;

	m_isVisible = ParseXmlAttribute(element, "isVisible", true);
	m_isSolid = ParseXmlAttribute(element, "isSolid", true);
	m_isOpaque = ParseXmlAttribute(element, "isOpaque", true);
	m_lightAmount = ParseXmlAttribute(element, "light", 0);
	floorWallUV = ParseXmlAttribute(element, "floorWallUV", Vec2());
	//floorWallUV.y -= 1;
	sideWallUV = ParseXmlAttribute(element, "sideWallUV", Vec2());
	//sideWallUV.y -= 1;
	topWallUV = ParseXmlAttribute(element, "topWallUV", Vec2());
	/*topWallUV.y -= 1;*/
	m_torchUvs.m_mins = ParseXmlAttribute(element, "torchMins", Vec2());
	m_torchUvs.m_mins.y = 1 - m_torchUvs.m_mins.y;
	m_torchUvs.m_maxs = ParseXmlAttribute(element, "torchMaxs", Vec2());
	m_torchUvs.m_maxs.y = 1 - m_torchUvs.m_maxs.y;
	m_floorWallUV = AABB2(floorWallUV, floorWallUV + 1.0f);
	m_sideWallUV = AABB2(sideWallUV, sideWallUV + 1.0f);
	m_topWallUV = AABB2(topWallUV, topWallUV + 1.0f);
	m_isTorch = m_name.find("Torch") != std::string::npos;
	m_isSpecular = ParseXmlAttribute(element, "isSpecular", false);
	return false;
}

void BlockDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument xml_doc;
	tinyxml2::XMLError result = xml_doc.LoadFile("Data/Definitions/BlockDefinitions.xml");
	if (result != tinyxml2::XML_SUCCESS)
		return;

	tinyxml2::XMLElement* rootElement = xml_doc.RootElement();
	if (rootElement != nullptr)
	{
		for (const tinyxml2::XMLElement* elements = rootElement->FirstChildElement(); elements; elements = elements->NextSiblingElement())
		{
			BlockDefinition* block = new BlockDefinition();
			block->LoadFromXmlElement(*elements);
			s_BlockDefinitions.push_back(block);
		}
	}
	return;
}

const BlockDefinition* BlockDefinition::GetByName(const std::string& name)
{
	for (int i = 0; i < (int)s_BlockDefinitions.size(); i++)
	{
		if (s_BlockDefinitions[i]->m_name == name)
		{
			return s_BlockDefinitions[i];
		}
	}
	return nullptr;
}

const BlockDefinition* BlockDefinition::GetByIndex(unsigned char index)
{
	if (index > (int)s_BlockDefinitions.size() || index == -1)
	{
		return nullptr;
	}
	else return s_BlockDefinitions[index];
}

const int BlockDefinition::GetIndexBasedOnName(const std::string& name)
{
	for (int i = 0; i < (int)s_BlockDefinitions.size(); i++)
	{
		if (s_BlockDefinitions[i]->m_name == name)
		{
			return i;
		}
	}
	return -1;
}


bool BlockDefinition::DoesEmitLight() const
{
	return (m_lightAmount != 0 || m_name == "Glowstone");
}

bool BlockDefinition::IsWater() const
{
	return m_name == "Water";
}

bool BlockDefinition::IsLava() const
{
	return m_name == "Lava";
}

bool BlockDefinition::IsTorch() const
{
	return m_isTorch;
}


bool BlockDefinition::IsSpecular() const
{
	return m_isSpecular;
}

EulerAngles BlockDefinition::GetTorchAngles() const
{
	EulerAngles returnValue;
	if (m_name == "FrontTorch")
	{
		returnValue.m_pitchDegrees = -m_torchAngle;
	}
	else if (m_name == "BackTorch")
	{
		returnValue.m_pitchDegrees = m_torchAngle;
	}
	else if (m_name == "LeftTorch")
	{
		returnValue.m_rollDegrees = m_torchAngle;
	}
	else if (m_name == "RightTorch")
	{
		returnValue.m_rollDegrees = -m_torchAngle;
	}
	return returnValue;
}