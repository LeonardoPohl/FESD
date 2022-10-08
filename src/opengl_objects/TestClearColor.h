#pragma once

#include <GLCore/GLObject.h>

namespace GLObject
{
	class TestClearColor : public GLObject
	{
	public:
		TestClearColor();
		~TestClearColor();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		float m_ClearColor[4];
	};
}