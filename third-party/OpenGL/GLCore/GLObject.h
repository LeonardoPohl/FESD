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
		GLObject();
		GLObject(Camera *cam);
		virtual ~GLObject() = default;

		virtual void OnUpdate() { }
		virtual void OnRender() { }
		virtual void OnImGuiRender() { }
	};

	class TestMenu : public GLObject
	{
	public:
		TestMenu(GLObject *&currentTestPointer);
		TestMenu(Camera *cam, GLObject *&currentTestPointer);

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
		GLObject*& m_CurrentTest;
		Camera *camera;
		std::vector<std::pair<std::string, std::function<GLObject*(Camera *)>>> m_Tests;
	};
}