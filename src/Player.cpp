#include "Player.h"
#include "InputHandler.h"



Player::Player()
{
	movementSpeed = 12.0f;
	turnSpeed = 3.0f;

	turretRotation = 0.0f;
	turretRotationSpeed = 5.0f;
}

Player::Player(mesh m, texture* t = nullptr, texture* n = nullptr) : Player()
{
	my_mesh = m;
	my_tex = t;
	my_normal = n;

	parent = nullptr;
}


Player::~Player()
{
}

void Player::Initialize()
{
	InputHandler::BindAxis(GLFW_KEY_UP, 1.0f, AxisDelegate::from_function<Player, &Player::MoveForwards>(this));
	InputHandler::BindAxis(GLFW_KEY_DOWN, -1.0f, AxisDelegate::from_function<Player, &Player::MoveForwards>(this));
	InputHandler::BindAxis(GLFW_KEY_LEFT, -1.0f, AxisDelegate::from_function<Player, &Player::TurnRight>(this));
	InputHandler::BindAxis(GLFW_KEY_RIGHT, 1.0f, AxisDelegate::from_function<Player, &Player::TurnRight>(this));
}

void Player::update(float delta_time) 
{
	RenderedObject::update(delta_time);

	if (turretRotation != 0.0f)
	{
		float deltaRotation = sign(turretRotation) * fmin<float>(abs(turretRotation), turretRotationSpeed) * delta_time;
		turretObject->get_mesh()->get_transform().rotate(vec3(0.0f, deltaRotation, 0.0f));

		turretRotation -= deltaRotation;
	}
}

void Player::SetTurretObject(RenderedObject * obj)
{
	turretObject = obj;
}

void Player::RotateTurret(float delta)
{
	turretRotation += delta;
}

void Player::MoveForwards(float delta)
{
	vec3 forward = my_mesh.get_transform().orientation * vec3(0.0f, 0.0f, -1.0f);
	my_mesh.get_transform().translate(forward * delta * movementSpeed * InputHandler::DeltaTime);
}

void Player::TurnRight(float delta)
{
	my_mesh.get_transform().rotate(vec3(0.0f, -delta * turnSpeed * InputHandler::DeltaTime, 0.0f));
}
