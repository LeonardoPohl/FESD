## Fusion of Multiple Lidars and Inertial Sensors for the Real-Time Pose Tracking of Human Motion

[[Fusion of Multiple Lidars and Inertial Sensors for the Real-Time Pose Tracking of Human Motion.pdf]]

## A Review: Point Cloud-Based 3D Human Joints Estimation

[[A_Review_Point_Cloud-Based_3D_Human_Joints_Estimat.pdf]]

### Important papers mentioned

4. [[A Depth Camera-based Human Activity Recognition via Deep Learning Recurrent Neural Network for Health and Social Care Services.pdf]]
7. Point cloud library
8. Open3D
9. [[Sensor fusion for 3D human body tracking with an Articulated 3D Body Model.pdf]]
18. [[Real-time Simultaneous Pose and Shape Estimation for Articulated Objects Using a Single Depth Camera.pdf]]
	1. single camera pose estimation
	2. uses first few frames to acquire human pose in the dynamic scenes -> usually does not provide complete information
19. 


### Summary

#### Introduction

- Human Centred applications of depth sensors
	- 3D Segmentation
	- Size-Measurement
	- 3D Reconstruction
	- Behaviour recognition
	- Health monitoring
- Conventional methods use 2D Images or videos for HPE
	- Progress made with deep learning
	- Complex backgrounds and variable viewpoints are limiting for 2D
- Process of 3D HPE
	- Data Acquisition using a depth sensors
		- RGB Data
		- Depth Map
		- Point cloud
	- Application of different methods
		- Template based
			- Creating Template
			- Matching Template
			- Matching Correlation
		- Feature based
			- Feature Detection
			- Feature Extraction
			- Feature Description
		- Machine Learning based
			- Input 3D data
			- Network Structure
			- Output Joint Position
	- Output: 3D position of joints
- Compared to meshes Point Clouds are smaller in storage and are easier to obtain
- Multiple Open source libraries are used
- Point Cloud Library (PCL)
	- Cross platform C++
	- large number of features
		- acquisition
		- filtering
		- segmentation
		- registration
		- visualisation
- Open3D
	- 3D data structure
	- 3D data processing algorithms
	- Scene reconstruction
	- 3D visualisation
- PCL is more mature
- Open3D can be installed and used in python and programming is faster and simpler
- Methods for HPE:
	- Template based
		- Geometric Model (i.e Cylinder Model)
		- Mathemetical Model (i.e. SoG model)
		- Mesh Model (i.e. SMPL model)
	- Feature based
		- Geodesic Distance
		- Geometric Feature (i.e Silhouette feature)
	- Machine Learning based
		- NN (i.e 3D CNN)
		- Classigiication Tree (i.e RF, RTW)

#### Depth Sensors for 3D Data Acquisition

-  3 different categories of depth cameras
	- Binocular Stereo vision
	- Time of flight (ToF)
	- Structured light
- In past more passive such as Binocular and then calculate the depth
	- This has some drawbacks regarding the texture of objects which messes up depth calculation and it is time consuming
- Lidar and radar are usually used more in military applications and in autonomous driving
- Taxonomy of Ranging Priciple
	- Structured Light
		- Speckle Structured Light
		- Fringe Structured Light
		- Coded Structured Light
	- ToF
		- Direct
		- Indirect
	- Binocular Stereo Vision
- ToF
	- Sends continous light puleses
	- Sensor receives light
	- Final distance is calculated when the flight time of the detected light pulse is obtained
	- Receiver needs to be highly accurate therefore miniturisation and cost reduction is difficult
	- Radar
		- milimeter waves
		- has strong anti-interference ability
	- Lidar
		- Emits laser signals
		- Has high detection accuracy
	- Advanteges
		- Long detection distance
		- large tolerance to ambient range
		- High frame rate
	- Disadvantage
		- High Equipment requirements
		- High resource consumption
		- low edge accuracy
	- E.g. Kinect v2 or Intel rS L515 (both available for thesis)
- Structured Light
	- Emits pseudo random light spots
	- Advantages
		- Miniturization
		- Low resource consumption
		- high resolution
	- Disadvantages
		- Small tolerance to ambient light
		- Short detection range
		-  high noise
	- E.g. Orbbec 

#### Methods of Point Cloud Based Joint Estimation

- Not focusing on RGB or Depth map
- Only Point cloud based methods
- Template Based Methods
	- judges motion category by comparing the plate based algorithms
	- Geometric Model
		- roughly divide the human body into several parts
		- usually cylinder ellipse or rectangles 
		- Head-Neck-Trunk (HNT)
		- Some methods (paper 15) uses face detection for 2D image first to segment the human
	- Mathematical model
		- transfer conceptual knowledge commonly used in mathematics to model construction
		- build a model with the representation of probability distribution to list each possible result and give their probabilities
		- mainly Gaussian Mixture Model (GMM)
			- Establish mixed model based on multiple gaussian distributions for each pixel of the image
			- parameters are continuously updated according to observed image
			- Background estimation is performed simultaneously
			- 


## Point2Skeleton: Learning Skeletal Representations from Point Clouds

[[Point2Skeleton_Learning_Skeletal_Representations_f.pdf]]