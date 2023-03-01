#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Texture.hpp"
#include <vector>
class Game;
class Props : public Entity
{
public:
	Props();
	~Props();
	Props(Game* game, Vec3 m_position, EulerAngles m_angularVelocity, std::vector<Vertex_PCU> verts);
	void				Render() const override;
	void				Update(float deltaSeconds) override;
	void				UpdateRotation(float deltaSeconds);
public:
	std::vector<Vertex_PCU>		vertices;
	Texture*					m_texture = nullptr;
	Rgba8						m_color;
	float						m_tintColor;
	float						m_colorChange = 0.0f;
	float						m_colorChangeDelta = 1.0f;
};

