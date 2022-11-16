
## TODO

### On the train

- [ ] Read [[A_Review_Point_Cloud-Based_3D_Human_Joints_Estimat.pdf]]
	- [ ] Write a summary and mark down every important Paper that is mentioned
- [ ] Read [[An Improved RANSAC for 3D Point Cloud Plane.pdf]]
- [ ] Read [[The Normal Distributions Transform A New Approach to Laser Scan.pdf]]
- [ ] Maybe write a bit thesis
- [ ] Implement Plane segmentation

## Recording and Playback

- [ ]  Record Orbbec Camera
- [ ]  Record Realsense Camera
- [ ]  Play back Orbbec Camera
- [ ]  Play back Realsense Camera

## Point clouds

- [ ] Join multiple Point clouds (Point Cloud registration)
	- [ ] Depth Calibration, a meter should be a meter regardless of RGBD camera
	- [ ] Implement NDT Cells
		- [ ] Test NDT validity according to [[An Improved RANSAC for 3D Point Cloud Plane.pdf]]
	- [ ] Do floor detection using NDT
	- [ ] Use NDT cells for Point cloud Registration: [[The Normal Distributions Transform A New Approach to Laser Scan.pdf]]
- [ ] #minor Find a metric to verify accuracy of Point cloud Registration

## Human Pose estimation

- [ ] #actual-thesis-work Find different methods for HPE in point clouds
- [ ] #actual-thesis-work Weasel yourself out of using RGB or grey scale images for HPE
- [ ] #actual-thesis-work Implement HPE for point clouds
- [ ] #actual-thesis-work Find metric to verify validity of method
- [ ] #actual-thesis-work Or use relative improvement as metric?
- [ ] #actual-thesis-work Compare single camera HPE from each angle to joined point cloud

## Thesis writing

- Introduction
	- [ ] SilverFit
	- [ ] RGBD cameras
		- [ ] Point cloud for different representations
	- [ ] Human Pose detection
- Previous Work #later
	- [ ] Camera Registration
	- [ ] Human Pose detection
- Prerequisites 
	- [ ] Point cloud extraction
		- [ ] Camera Calibration Extrinsics and Intrinsic for reverse projection
		- [ ] RGB -> Point Cloud
	- [ ] NDT cells
		- [ ] Plane Segmentation
		- [ ] Floor detection using
			- [ ] Why is floor detection important
		- [ ] Point cloud registration 
- Human Pose Estimation
	- TODO
	- [[A_Review_Point_Cloud-Based_3D_Human_Joints_Estimat.pdf]]
- Experiments
	- TODO
	- [ ] Experiment Setup
	- [ ] What conclusions could be deduced
- Results
	- TBD
- Conclusion
	- TBD
- Future Work
	- [ ] Many Performance improvements
	- [ ] Improved Graphics
		- [ ] Performance
		- [ ] Frame Buffers
	- [ ] Improved experiments
	- [ ] Floor detection as an useful feature can be improved

## Slight improvements #minor 

- [ ] Improve NDT cell detection flow, should be together rather than separate; one button does everything if button is even needed
- [ ] Rename Project and Repository
- [ ] Rewrite Readme
- [ ] Refactor code
	- [ ] Rename Variables
	- [ ] Comment Code
- [ ] ImGui Docking not working