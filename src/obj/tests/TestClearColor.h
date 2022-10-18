#pragma once

#include <GLCore/GLObject.h>

namespace GLObject
{
	class TestClearColor : public GLObject
	{
	public:
		TestClearColor(Arguments *args = nullptr) : GLObject(args) { }
		TestClearColor(Camera *cam, Arguments *args = nullptr) : GLObject(cam, args) { }

		void OnStart() override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		float m_ClearColor[4];
	};
}