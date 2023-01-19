
## 21-11-2022

- Want to start finishing my thesis
- I don't see an end coming closer
- I have been working on preliminaries for such a long time
- Current version still not working fully
	- Multiple Point clouds not being shown
	- Still working on recording and playback
	- Still need a way to reliably create a ground truth 
	- Idea:
		- Go through every frame and show 3 different angles
		- Make the user select every joint
		- Using multiple viewing angles an exact location of the joint can be determined
- New scope maybe:
	- The effect of multi modal cameras on the 2 most promising
		- Template Based Methods
		- Feature Based Methods
		- Machine Learning based models
	- How many recordings should I create?
	- How much overlap should there be between cameras?
	- Some methods improve using multiple frames how should I consider this in the measurement?
		- Should single frame performance be important?
			- Most RGB-D applications are not still images
		- Should I use the average accuracy?
		- Should I use a delayed average for the methods that improve over time?
	- How big should the dataset be?
	- Can I use pretrained models?
	- How can I create ground truth fast? 
		- Is there a better way other than manual?
	- If I have to manually create the ground truth this will take another couple months, could we somehow reformulate the problem so it only takes another 2 months?
 
## 19-12-2022

- What to work on:
	1. Playback RS
	2. Recording Orbbec
	3. Playback Orbbec
	4. Skeleton Detection in recording stream
- Research Focus
	- When does HPE go wrong?
	- Detect Scenarios where it goes wrong.
	- Dataset contains of depth and rgb stream and skeleton ground truth

## 19-01-2022

### Done since last meeting

- Playback and recording working for RS and Orbbec
	- RS quite large compared to Orbbec
- Improved framerate -> recording at 30fps for 2 cameras
- Couldn't find a satisfactory skeleton detector for points
	- Using OpenPose
		- Requires RGB stream but results are good
		- Using the points I can project them in 3d and at the data validation stage either complete skeleton or form the average 
		- not fully done storing it
		- Should I store in 3d or in 2d, or both?
		- Will calculate on finished recordings to keep good framerate
			  ->  Recordings will be at 30FPS -> 100s = 3000 Frames (Good enough?)
- Working on Point cloud segmentation
	- NDT - Normal-Distributions Transform
		- thought to use before 
		- But did get good results
	- CPD - Coherent Point Drift
		- Wont implement but will discuss usage in thesis
	- ICP - using it
		- Not fully implemented
		- Build KD-Tree
		- ICP
			- Find nearest neighbour for each point
			- Propose a rotation and translation according to the mean square error
			- Repeat
		- Store translation and rotation with recording because this takes quite a while to compute

### Plans for continuing

- I would like to be done by the end of February
- Implementation left (next week done)
	- ICP
	- Openpose recording
- Recording
	- How much should I record for a good dataset?
	- How often should I repeat the same action?
	- Should I make the light conditions optimal, or record different light settings?
	- Should I complete incomplete skeletons with the second skeleton if available?
- Data Labelling and data manipulation
	- Open pose uses confidence I will use that for preference for moving/removing
- Train and evaluate NN
	- What type of neural network
		- Image processing uses CNNs a lot should I also use a CNN?
		- Should the training only involve the Joint data?
			- Might be easier to train and might already offer good results
			- Might not be enough
		- Train the network on the confidence? Have it estimate the confidence level based on position, RGB and Depth
	- What should be the split for training and testing/evaluating
		- Read around 60-80% for training and the rest for testing/evaluating
- Thesis Outline
	- Introduction
		- Research question
	- Previous Work
		- Different Data acquisition tools, PCL, Nuitrack
		- Camera Drivers, Openni, Librealsense
		- Skeleton Detectors
			- 3D and 2D
		- Fault Detectors(?)
	- Data acquisition Tool FESD
		- Different types of streams
		- (short) discussion of Point cloud visualisation
		- Data Recording and synchronisation
		- Skeleton detection using OpenPose
	- Data Labelling and Manipulation
		- Facts about recordings
			- How many frames
			- What frame rate
			- How much error
			- ...
		- Data Labelling, completion
			- 3D average of joints?
			- Skeleton completion using 2 separate Perspectives?
		- Data Manipulation
			- Move joints in 3D and lower confidence
	- Fault Estimation
		- Neural Network
			- Which architectures were considered
			- Why did I choose ...?
		- Training
			- What is trained on
		- Evaluating
			- How does the evaluation take place
	- Results
		- How accurate is the network?
		- What can the network do?
	- Conclusion
		- Answer research question "Can we detect errors in a Human skeleton using multiple RGBD Cameras?"
	- Future Work
- How long should the thesis be?
- How does the defence work?