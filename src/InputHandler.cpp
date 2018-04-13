#include "InputHandler.h"

map<int, bool> InputHandler::keys;
queue<key_event> InputHandler::unhandled_keys;
multimap<int, pair<int, KeyDelegate>> InputHandler::keyMap;
multimap<int, pair<float, AxisDelegate>> InputHandler::axisMap;
double InputHandler::mouse_x, InputHandler::mouse_y, InputHandler::previous_x, InputHandler::previous_y;
float InputHandler::timeSinceMouseMovement = 0.0f;
float InputHandler::DeltaTime = 0.0f;

InputHandler::InputHandler()
{
	GetMousePos(&previous_x, &previous_y);
}


InputHandler::~InputHandler()
{
}

void InputHandler::Update(float deltaTime)
{
	DeltaTime = deltaTime;
	timeSinceMouseMovement += deltaTime;

	while (!unhandled_keys.empty()) {
		// Get keys from the queue
		key_event event = unhandled_keys.front();
		unhandled_keys.pop();

		// Handle key press/release
		pair<multimap<int, pair<int, KeyDelegate>>::iterator, multimap<int, pair<int, KeyDelegate>>::iterator> boundFunctions;

		// Find iterators
		boundFunctions = keyMap.equal_range(event.key);

		// Go through bound functions and run them
		for (multimap<int, pair<int, KeyDelegate>>::iterator it = boundFunctions.first; it != boundFunctions.second; ++it)
		{
			pair<int, KeyDelegate> func = it->second;
			if (func.first == event.action)
			{
				func.second();
			}
		}

		// Set a flag for held buttons to run axis functions on them
		keys[event.key] = event.action == GLFW_PRESS || event.action == GLFW_REPEAT;
	}

	// Handle axis
	for (auto keyevent : keys)
	{
		// If key is not held skip it
		if (!keyevent.second)
		{
			continue;
		}

		// Create a pair of iterators to look for any functions bound to selected key
		pair<multimap<int, pair<float, AxisDelegate>>::iterator, multimap<int, pair<float, AxisDelegate>>::iterator> boundAxis;

		// Find iterators
		boundAxis = axisMap.equal_range(keyevent.first);

		// Go through bound functions and run them
		for (multimap<int, pair<float, AxisDelegate>>::iterator it = boundAxis.first; it != boundAxis.second; ++it)
		{
			pair<float, AxisDelegate> func = it->second;
			func.second(func.first);
		}
	}
}

// Gets mouse delta taking screen aspect into account
void InputHandler::GetRealMouseDelta(double* xdelta, double* ydelta)
{
	static double ratio_width = quarter_pi<float>() / static_cast<float>(renderer::get_screen_width());
	static double ratio_height =
		(quarter_pi<float>() *
		(static_cast<float>(renderer::get_screen_height()) / static_cast<float>(renderer::get_screen_width()))) /
		static_cast<float>(renderer::get_screen_height());

	double dx, dy;
	InputHandler::GetMouseDelta(&dx, &dy);

	*xdelta = dx * ratio_width;
	*ydelta = dy * ratio_height;
}

// Gets mouse delta
void InputHandler::GetMouseDelta(double * xdelta, double * ydelta)
{
	if (timeSinceMouseMovement < 0.05f)
	{
		*xdelta = mouse_x - previous_x;
		*ydelta = mouse_y - previous_y;
	}
	else {
		*xdelta = 0.0f;
		*ydelta = 0.0f;
	}
}

// Gets mouse position
void InputHandler::GetMousePos(double * xpos, double * ypos)
{
	*xpos = mouse_x;
	*ypos = mouse_y;
}

// Handles keyboard input
void InputHandler::KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Just add the necessary data into a queue
	unhandled_keys.emplace(key, scancode, action, mods, std::chrono::steady_clock::now());
}

// Handles mouse position
void InputHandler::MousePosHandler(GLFWwindow * window, double xpos, double ypos)
{
	previous_x = mouse_x;
	previous_y = mouse_y;
	mouse_x = xpos;
	mouse_y = ypos;
	timeSinceMouseMovement = 0.0f;
}

// Handles mouse buttons (or does it?)
void InputHandler::MouseButtonHandler(GLFWwindow * window, int button, int action, int mods)
{

}