#pragma once

#include <GLCore/GLObject.h>

namespace GLObject
{
	class TestClearColor : public GLObject
	{
	public:
		TestClearColor(const Camera *cam = nullptr);

		void OnRender() override;
		void OnImGuiRender() override;
	private:
		float m_ClearColor[4];
	};
}