#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>

namespace GLObject
{
	class GLObject
	{
	public:
		GLObject() {}
		virtual ~GLObject() {}

		virtual void OnUpdate(float deltaTime = 0) {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
	};

	class TestMenu : public GLObject
	{
	public:
		TestMenu(GLObject *&currentTestPointer);

		void OnImGuiRender() override;

		template<typename T>
		void RegisterTest(const std::string &name)
		{
			std::cout << "Registering test " << name << std::endl;
			m_Tests.push_back(std::make_pair(name, []()
											 {
												 return new T();
											 }));
		}
	private:
		GLObject*& m_CurrentTest;
		std::vector<std::pair<std::string, std::function<GLObject*()>>> m_Tests;
	};
}