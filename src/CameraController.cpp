#include "CameraController.h"

CameraController::CameraController()
{
}

CameraController::~CameraController()
{

}

void CameraController::Initialize()
{
	// Create cameras with helper functions
	staticCam = CreateTargetCamera("targetCam");
	staticCam->set_position(vec3(0.0f, 10.0f, 50.0f));
	staticCam->set_target(vec3(0.0f, 0.0f, 0.0f));

	freeCam = CreateFreeCamera("freeCam");
	freeCam->set_position(vec3(0.0f, 10.0f, 50.0f));
	freeCam->set_target(vec3(0.0f, 0.0f, 0.0f));
	
	// Set active camera number to 0
	activeCamera = 0;

	// Bind camera switching functions to proper keys
	InputHandler::BindKey('1', GLFW_PRESS, KeyDelegate::from_function<CameraController, &CameraController::PreviousCamera>(this));
	InputHandler::BindKey('2', GLFW_PRESS, KeyDelegate::from_function<CameraController, &CameraController::NextCamera>(this));

	InputHandler::BindAxis('W', 1.0f, AxisDelegate::from_function<CameraController, &CameraController::MoveForward>(this));
	InputHandler::BindAxis('S', -1.0f, AxisDelegate::from_function<CameraController, &CameraController::MoveForward>(this));
	InputHandler::BindAxis('A', -1.0f, AxisDelegate::from_function<CameraController, &CameraController::MoveRight>(this));
	InputHandler::BindAxis('D', 1.0f, AxisDelegate::from_function<CameraController, &CameraController::MoveRight>(this));
}

camera* CameraController::GetActiveCamera()
{
	return cameras[activeCamera];
}

camera* CameraController::GetCameraByName(string name)
{
	return _cameras[name];
}

void CameraController::NextCamera()
{
	if (++activeCamera >= cameras.size())
	{
		activeCamera = 0;
	}
}

void CameraController::PreviousCamera()
{
	if (--activeCamera < 0 )
	{
		activeCamera = cameras.size() - 1;
	}
}

void CameraController::MoveForward(float value)
{
	if( GetActiveCamera() == freeCam )
		freeCam->move(vec3(0.0f, 0.0f, value) * cameraMovementSpeed * InputHandler::DeltaTime);
}

void CameraController::MoveRight(float value)
{
	if (GetActiveCamera() == freeCam)
		freeCam->move(vec3(value, 0.0f, 0.0f) * cameraMovementSpeed * InputHandler::DeltaTime);
}

void CameraController::HandleFreeCameraRotation()
{
	// Get mouse delta from InputHandler
	double delta_x, delta_y;
	InputHandler::GetRealMouseDelta(&delta_x, &delta_y);

	float deltaTime = InputHandler::DeltaTime;

	// Multiply deltas by ratios and mouse sensitivity
	delta_x *= mouseSensitivity * deltaTime;
	delta_y *= mouseSensitivity * deltaTime;

	// Rotate cameras by delta
	// delta_y - x-axis rotation
	// delta_x - y-axis rotation
	freeCam->rotate(delta_x, -delta_y);
}

double CameraController::GetMouseSensitivity()
{
	return mouseSensitivity;
}

void CameraController::SetMouseSensitivity(double sens)
{
	mouseSensitivity = sens;
}

void CameraController::AddNewCamera(string name, camera * cam)
{
	if (_cameras.find(name) != _cameras.end())
	{
		cout << "Camera with name " << name << " already exists!";
		return;
	}

	_cameras[name] = cam;
	cameras.emplace(cameras.end(), cam);
}

target_camera* CameraController::CreateTargetCamera(string name)
{
	if (_cameras.find(name) != _cameras.end())
	{
		cout << "Camera with name " << name << " already exists!";
		return nullptr;
	}

	target_camera* newCamera = new target_camera();
	newCamera->set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	_cameras[name] = newCamera;
	cameras.emplace(cameras.end(), newCamera);
	return newCamera;
}

chase_camera* CameraController::CreateChaseCamera(string name)
{
	if (_cameras.find(name) != _cameras.end())
	{
		cout << "Camera with name " << name << " already exists!";
		return nullptr;
	}

	chase_camera* newCamera = new chase_camera();
	newCamera->set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	_cameras[name] = newCamera;
	cameras.emplace(cameras.end(), newCamera);
	return newCamera;
}

free_camera* CameraController::CreateFreeCamera(string name)
{
	if (_cameras.find(name) != _cameras.end())
	{
		cout << "Camera with name " << name << " already exists!";
		return nullptr;
	}

	free_camera* newCamera = new free_camera();
	newCamera->set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	_cameras[name] = newCamera;
	cameras.emplace(cameras.end(), newCamera);
	return newCamera;
}