#include "Entity.hpp"
#include "math.h"
#include "Game/GameCommon.hpp"
class Game;
Entity::Entity(Game* game, Vec2 position)
{
	this->game = game;
	this->m_position2D = position;
}

Entity::Entity(Game* game, Vec3 position, EulerAngles angularVelocity)
{
	this->game = game;
	m_position = position;
	m_angularVelocity = angularVelocity;
}

Mat44 Entity::GetModalMatrix() const
{
	Mat44 translationMatrix = Mat44::CreateTranslation3D(m_position);
	Mat44 rotationMatrix = m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp();
	translationMatrix.Append(rotationMatrix);
	return translationMatrix;
}

Entity::Entity()
{
	
}

void Entity::Update(float deltaSeconds)
{
	deltaSeconds = deltaSeconds;
}

void Entity::Render() const
{
}

void Entity::DebugRender() const
{
}


Vec2 Entity::GetForwardNormal() const
{
	float deg = m_orientationDegrees2D;
	float x =  cosf(deg * (3.1415f / 180.f));
	float y = sinf(deg * (3.1415f / 180.f));

	return Vec2(x, y);
}



