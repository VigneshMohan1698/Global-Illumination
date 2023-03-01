#include "Player.hpp"
#include "Game/Game.hpp"
#include "Engine/Math/MathUtils.hpp"
Player::Player()
{
}

Player::~Player()
{
}

Player::Player(Game* game, Vec3 position, EulerAngles angularVelocity) : Entity(game, position, angularVelocity)
{

}

void Player::Render() const
{
}

void Player::Update(float deltaSeconds)
{
	UNUSED((void)deltaSeconds);
	ClampPlayerRotation();
	this->game->m_worldCamera.SetTransform(m_position, m_orientationDegrees);
}

void Player::ClampPlayerRotation()
{
	m_orientationDegrees.m_pitchDegrees= Clamp(m_orientationDegrees.m_pitchDegrees, -85.0f, 85.0f);
	m_orientationDegrees.m_rollDegrees = Clamp(m_orientationDegrees.m_rollDegrees, -45.0f, 45.0f);
}
