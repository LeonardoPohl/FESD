#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <cstdarg>
#include "Camera.h"


namespace GLObject
{
	struct Arguments { };
	class GLObject
	{
	public:
		GLObject(Arguments *args = nullptr)
		{
			if (args)
				OnStart(args);
			else
				OnStart();
		}

		GLObject(Camera *cam, Arguments *args = nullptr) : camera(cam)
		{
			if (args)
				OnStart(args);
			else
				OnStart();
		}

		virtual ~GLObject() = default;

		virtual void OnStart() { }
		virtual void OnStart(Arguments *args) { }
		virtual void OnUpdate() { }
		virtual void OnRender() { }
		virtual void OnImGuiRender() { }
	protected:
		Camera *camera;
	};

	struct TestMenuArguments : Arguments
	{
		GLObject *currentTestPointer;

		TestMenuArguments(GLObject *currentTestPointer) : currentTestPointer(currentTestPointer) { }
	};

	class TestMenu : public GLObject
	{
	public:
		TestMenu(Arguments *args = nullptr) : GLObject(args) { }
		TestMenu(Camera *cam, Arguments *args = nullptr) : GLObject(cam, args) { }

		void OnStart(Arguments *args) override;
		void OnImGuiRender() override;

		template<typename GLObject>
		void RegisterTest(const std::string &name)
		{
			std::cout << "Registering test " << name << std::endl;
			m_Tests.push_back(std::make_pair(name, [](Camera *cam)
											 {
												 return new GLObject(cam);
											 }));
		}
	private:
		GLObject* m_CurrentTest{};
		std::vector<std::pair<std::string, std::function<GLObject*(Camera *)>>> m_Tests;
	};
}