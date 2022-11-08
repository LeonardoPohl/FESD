#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <array>
#include <memory>
#include <cameras/DepthCamera.h>

#include <unordered_map>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utilities/Point.h"
#include "utilities/Plane.h"
#include "utilities/Cell.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud(DepthCamera *depthCamera, const Camera *cam = nullptr, Renderer *renderer = nullptr);

		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		void pauseStream()
		{
			m_StateElem = 1;
			m_State = IDLE;
		}

		void resumeStream()
		{
			m_StateElem = 0;
			m_State = STREAM;

			for (auto key : m_pCellByKey)
				delete key.second;

			m_pCellByKey.clear();
			m_ColorBypCell.clear();
			mp_PlanarCells.clear();
			mp_NonPlanarPoints.clear();
		}

		void streamDepth(int i, const int16_t *depth);
		void startNormalCalculation();
		void calculateNormals(int i);
		void startCellAssignment();
		void assignCells(int i, glm::vec3 cellSize);
		void startCellCalculation();
		void calculateCells(int i, glm::vec3 cellSize);

		enum State
		{
			STREAM,
			IDLE,
			NORMALS,
			CELLS,
			CALC_CELLS
		};

		static const int m_StateCount = 6;
		static const char *m_StateNames[];

		State m_State{ STREAM };
		int m_StateElem{ 0 };

		DepthCamera *mp_DepthCamera;

		Point *m_Points; 
		Point::Vertex *m_Vertices; 

		Renderer *mp_Renderer;
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		float m_RotationFactor{ 0 };
		glm::vec3 m_Rotation{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Translation{ 0.f, 0.f, 0.f };
		glm::vec3 m_ModelTranslation{ 0.0f };

		float m_Scale{ 1.0f };
		float m_Depth_Scale {5.0f};

		std::default_random_engine m_Generator;
		std::unique_ptr<std::uniform_int_distribution<int>> m_Distribution{};

		Point::CMAP m_CMAP{ Point::CMAP::VIRIDIS };
		int m_CMAPElem{ 0 };

		std::vector<std::pair<Plane, int>> m_PointCountByPlane;
		int m_MaxPointCount{ 0 };

		int m_OctTreeDevisions{ 200 };
		int m_NumElements{ 0 };
		int m_StreamWidth{ 0 };
		int m_StreamHeight{ 0 };

		std::unordered_map<Cell*, glm::vec3> m_ColorBypCell;
		std::unordered_map<std::string, Cell*> m_pCellByKey;

		std::vector<Cell *> mp_PlanarCells;
		std::vector<Point *> mp_NonPlanarPoints;

		// TODO low prio: draw bounding box
		glm::vec3 m_MinBoundingPoint{};
		glm::vec3 m_MaxBoundingPoint{};

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };
	};
};
