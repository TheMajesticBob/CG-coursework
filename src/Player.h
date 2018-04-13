#pragma once
#include "RenderedObject.h"

class Player :
	public RenderedObject
{
public:
	Player();
	Player(mesh m, effect *e, texture *t, texture *n);
	~Player();

	void Initialize();

	void update(float delta_time) override;
	
	void SetTurretObject(RenderedObject* obj);
	void RotateTurret(float delta);

	void MoveForwards(float delta);
	void TurnRight(float delta);

private:
	float movementSpeed;
	float turnSpeed;

	float turretRotation;
	float turretRotationSpeed;

	RenderedObject* turretObject;
};

