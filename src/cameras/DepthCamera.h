#pragma once
#include <stdexcept>
#include <string>
#include <GLCore/GLObject.h>
#include <json/json.h>

namespace GLObject
{
	class PointCloud;
}

enum class INTRINSICS
{
	FX,
	FY,
	CX,
	CY
};


class DepthCamera {
public:
	virtual ~DepthCamera() = default;

	/// <summary>
	/// Gets current depth frame
	/// </summary>
	/// <returns>Pointer to first depth pixel</returns>
	virtual const void *getDepth() = 0;

	/// <summary>
	/// Gets the Name of the Camera
	/// </summary>
	/// <returns>Name of Camera</returns>
	virtual std::string getName() const = 0; 

	/// <summary>
	/// Get Width in Pixel
	/// </summary>
	virtual unsigned int getDepthStreamWidth() const = 0;

	/// <summary>
	/// Get Height in Pixel
	/// </summary>
	virtual unsigned int getDepthStreamHeight() const = 0;

	/// <summary>
	/// Call update Functions
	/// </summary>
	virtual void OnUpdate() = 0;

	/// <summary>
	/// Execute Render
	/// </summary>
	virtual void OnRender() = 0;

	/// <summary>
	/// Execute ImGui Render
	/// </summary>
	virtual void OnImGuiRender() = 0;

	/// <summary>
	/// Start recording to file
	/// </summary>
	/// <param name="sessionName">Name of the session used for file naming and multi file synchronisation</param>
	/// <param name="numFrames">Number of Frames, if 0 recording has to be manually stopped</param>
	/// <returns>Path to saved file</returns>
	virtual std::string startRecording(std::string sessionName, unsigned int numFrames = 0) = 0;

	/// <summary>
	/// Save a single frame at the current time
	/// </summary>
	virtual void saveFrame() = 0;

	/// <summary>
	/// Stop recording
	/// </summary>
	virtual void stopRecording() = 0;
	
	virtual void showCameraInfo() = 0;

	virtual float getIntrinsics(INTRINSICS intrin) const = 0;
	virtual glm::mat3 getIntrinsics() const = 0;

	/// <returns>Window Name (Display: *Camera Name*)</returns>
	inline std::string getWindowName() const {
		return "Display: " + this->getCameraName();
	}

	/// <returns>Camera Name</returns>
	inline std::string getCameraName() const {
		return this->getName() + " Camera " + std::to_string(this->m_CameraId);
	}

	/// <returns>Window Name</returns>
	inline unsigned int getCameraId() const
	{
		return m_CameraId;
	}

	/// <returns>Camera Config</returns>
	inline Json::Value getCameraConfig() const
	{
		return m_CameraInfromation;
	}

	bool m_isEnabled{ false };
	bool m_selectedForRecording{ true };
protected:
	unsigned int m_CameraId;

	// For Later
	int m_playbackOffset{ 0 };

	Json::Value m_CameraInfromation;
};
