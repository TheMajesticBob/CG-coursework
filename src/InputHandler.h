#pragma once
#include <queue>
#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

// Class holding a pointer to object instance and a member method of the class in order to call it from InputController when needed
class KeyDelegate
{
	typedef void(*Type)(void* callee);

public:
	KeyDelegate(void* callee, Type function)
		: fpCallee(callee)
		, fpCallbackFunction(function) {}

	// Initialize the delegate with a method
	template <class T, void (T::*TMethod)()>
	static KeyDelegate from_function(T* callee)
	{
		KeyDelegate d(callee, &methodCaller<T, TMethod>);
		return d;
	}

	// Execute method
	void operator()() const
	{
		return (*fpCallbackFunction)(fpCallee);
	}
private:
	void* fpCallee;
	Type fpCallbackFunction;

	// Helper method casting method to proper type
	template <class T, void (T::*TMethod)()>
	static void methodCaller(void* callee)
	{
		T* p = static_cast<T*>(callee);
		return (p->*TMethod)();
	}
};

class AxisDelegate
{
	typedef void(*FloatArgType)(void* callee, float);

public:
	AxisDelegate(void* callee, FloatArgType function)
		: fpCallee(callee)
		, fpCallbackFunction(function) {}

	// Initialize the delegate with a method
	template <class T, void (T::*TMethod)(float)>
	static AxisDelegate from_function(T* callee)
	{
		AxisDelegate d(callee, &methodCaller<T, TMethod>);
		return d;
	}

	void operator()(float arg) const
	{
		return (*fpCallbackFunction)(fpCallee, arg);
	}
private:
	void* fpCallee;
	FloatArgType fpCallbackFunction;

	// Helper method casting method to proper type
	template <class T, void (T::*TMethod)(float)>
	static void methodCaller(void* callee, float arg)
	{
		T* p = static_cast<T*>(callee);
		return (p->*TMethod)(arg);
	}
};

struct key_event {
	int key, code, action, modifiers;
	std::chrono::steady_clock::time_point time_of_event;

	key_event(int k, int c, int a, int m, std::chrono::steady_clock::time_point t)
	{
		key = k;
		code = c;
		action = a;
		modifiers = m;
		time_of_event = t;
	}
};

class InputHandler
{
	public:
		InputHandler();
		~InputHandler();

		static void GetRealMouseDelta(double* xdelta, double* ydelta);
		static void GetMouseDelta(double* xdelta, double* ydelta);
		static void GetMousePos(double* xpos, double* ypos);

		// Input callbacks
		static void KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void MousePosHandler(GLFWwindow* window, double xpos, double ypos);
		static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods);

		static void Update(float deltaTime);
		static float DeltaTime;

		// Method binding a key action to a delegate
		static void BindKey(int key, int action, KeyDelegate d)
		{
			keyMap.emplace(key, pair<int, KeyDelegate>(action, d));
		}

		static void BindAxis(int key, float value, AxisDelegate d)
		{
			axisMap.emplace(key, pair<float, AxisDelegate>(value, d));
		}
	
	private:
		// Map holding pairs of key actions and their respective methods called when key state changes
		static multimap<int, pair<int, KeyDelegate>> keyMap;
		static multimap<int, pair<float, AxisDelegate>> axisMap;

		static double mouse_x, mouse_y, previous_x, previous_y;
		static float timeSinceMouseMovement;

		static map<int, bool> keys;
		static queue<key_event> unhandled_keys;
};