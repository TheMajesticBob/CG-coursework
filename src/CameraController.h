#pragma once
#include <glm\glm.hpp>
#include <graphics_framework.h>
#include "InputHandler.h"

using namespace std;
using namespace glm;
using namespace graphics_framework;

class CameraController
{
public:
	CameraController();
	~CameraController();

	void Initialize();

	camera* GetActiveCamera();
	camera* GetCameraByName(string name);

	target_camera* CreateTargetCamera(string name);
	chase_camera* CreateChaseCamera(string name);
	free_camera* CreateFreeCamera(string name);

	void HandleFreeCameraRotation();

	double GetMouseSensitivity();
	void SetMouseSensitivity(double sens);

	void AddNewCamera(string name, camera* cam);

private:
	int activeCamera;
	double mouseSensitivity = 80.0f;
	float cameraMovementSpeed = 15.0f;

	vector<camera*> cameras;
	map<string, camera*> _cameras;

	target_camera* staticCam;
	chase_camera chaseCam;
	free_camera* freeCam;

	void NextCamera();
	void PreviousCamera();

	void MoveForward(float value);
	void MoveRight(float value);
};