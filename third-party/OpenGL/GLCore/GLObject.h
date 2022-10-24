#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <cstdarg>
#include "Camera.h"


namespace GLObject
{
	class GLObject
	{
	public:
		GLObject(const Camera *cam = nullptr) : camera(cam) { }
		virtual ~GLObject() = default;

		virtual void OnUpdate() { }
		virtual void OnRender() { }
		virtual void OnImGuiRender() { }
	protected:
		const Camera *camera{nullptr};

	};

	class TestMenu : public GLObject
	{
	public:
		TestMenu(GLObject *&currentTestPointer, const Camera *cam = nullptr);

		void OnImGuiRender() override;

		template<typename GLObject>
		void RegisterTest(const std::string &name)
		{
			std::cout << "Registering test " << name << std::endl;
			m_Tests.push_back(std::make_pair(name, [](const Camera *cam)
											 {
												 return new GLObject(cam);
											 }));
		}
	private:
		GLObject*& m_CurrentTest;
		std::vector<std::pair<std::string, std::function<GLObject*(const Camera *)>>> m_Tests;
	};
}