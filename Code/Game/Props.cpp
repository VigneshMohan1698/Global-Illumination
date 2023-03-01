#include "Props.hpp"
#include "Engine/Renderer/RendererD12.hpp"
#include "Game/App.hpp"
#include "Engine/Math/VertexUtils.hpp"
#include <Engine/Math/MathUtils.hpp>

extern RendererD12* g_theRenderer;
extern App* g_theApp;

Props::Props()
{
	m_texture = nullptr;
	m_color = Rgba8::WHITE;
}

Props::~Props()
{
}

Props::Props(Game* game, Vec3 position, EulerAngles angularVelocity, std::vector<Vertex_PCU> verts) : Entity(game,position, angularVelocity)
{
	vertices = verts;
}


void Props::Render() const
{
	float modelColor[4];
    m_color.GetAsFloats(modelColor);
	//g_theRenderer->SetModalMatrix(GetModalMatrix());
	//g_theRenderer->SetModalColor(modelColor);
	//g_theRenderer->DrawVertexArray((int)vertices.size(), vertices.data());
}

void Props::Update(float deltaSeconds)
{
	UpdateRotation(deltaSeconds);
	if(m_colorChange != 0.0f)
	{
		float ColorTint = SinDegrees(m_colorChange);
		if (m_colorChange >= 360 || m_colorChange < 100.0f)
		{
			m_colorChangeDelta = -m_colorChangeDelta;
		}
		m_colorChange += 0.2f;
		m_tintColor = RangeMap(ColorTint, 0.0f, 1.0f,50.0f, 255.0f);
		float tints[4];
		m_color = Rgba8((unsigned char)m_tintColor, (unsigned char)m_tintColor, (unsigned char)255.0f, (unsigned char)255.0f);
		m_color.GetAsFloats(tints);
	/*	g_theRenderer->SetModalColor(tints);*/
	}
	
}
void Props::UpdateRotation(float deltaSeconds)
{
	m_orientationDegrees.m_yawDegrees += m_angularVelocity.m_yawDegrees * deltaSeconds;
	m_orientationDegrees.m_pitchDegrees += m_angularVelocity.m_pitchDegrees * deltaSeconds;
}
