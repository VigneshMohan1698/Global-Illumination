#pragma once
#include "Game/Entity.hpp"
class Player : public Entity
{
public:
	Player();
	~Player();
	Player(Game* game, Vec3 position, EulerAngles angularVelocity);
	void				Render() const;
	void				Update(float deltaSeconds);
	void				ClampPlayerRotation();
};

