#pragma once
#include "Engine/Core/Vertex_PCU.hpp"
#include "Game/GameCommon.hpp"
#include <Engine/Math/EulerAngles.hpp>
#include "Engine/Math/Mat44.hpp"
class Game;
class  Entity
{
public:
	Entity();
	Entity(Game* game, Vec2 position);
	Entity(Game* game,Vec3 position, EulerAngles m_angularVelocity);
	~Entity() {}
	virtual Mat44			GetModalMatrix() const;
	Rgba8					m_modelColor;

	virtual void			Update(float deltaSeconds);
	virtual void			Render() const;
	virtual void			DebugRender() const;

	Game*					game = nullptr;
	Vec2					GetForwardNormal() const;
	Vec2					m_position2D;
	float					m_orientationDegrees2D;

	Vec3					m_position;
	EulerAngles				m_angularVelocity;
	EulerAngles				m_orientationDegrees;

	float					m_ageInSeconds = 0.0f;
	float					m_physicsRadius = 0.0f;
	float					m_cosmeticRadius = 0.0f;
	bool					m_isGarbage = false;
	 
};