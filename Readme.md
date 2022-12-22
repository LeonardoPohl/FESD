# FESD - Fault estimation for skeleton detection

FESD is a fault detector that was developed in the scope of a master thesis at the University of Leiden. Its goal is to detect faults in skeleton detection using multi modal RGBD Cameras. The cameras that are currently used are the Orbbec Astra Pro and the Realsense L515 but it should not be hard to extend the code to handle different cameras. Openni2 is used as an interface to stream, record and replay the Depth stream of the Astra Camera. The excellent Realsense API is used to do the same for the realsense camera.

The project consists of two or three parts. The first is data aquisition. I implemented methods to display the depth data as a pointcloud and supply several settings for the recording process. This part of the project also supplies a playback functionallity to review the previously recorded sessions. It will also have a functionality to align multiple camera streams using NDT cells.

Skeleton detection is then performed and also stored along the the data stream. We assume that the detected skeleton is the ground truth. Before using the data for training the data is examined for any major faults. To emulate faulty data we randomly move the most likely joints to be faulty to a random position, which is a common fault which might be detectable. Using this method we try to make the training dataset artificially larger.

The fault estimation is then done using a Neural Network which is trained in python.

## Installation

To use the code in this repository some things have to be installed, this might not be a complete list.

- Orbbec Camera Driver - [Orbbec Download Page](https://orbbec3d.com/index/download.html)
- Orbbec OpenNI SDK - [Orbbec Download Page](https://orbbec3d.com/index/download.html)

## Near Future Work (TODOs)

- Multiple pointclouds visible at same time
- Camera stream alignment using NDT cells
- **Skeleton detection and recording**
- Write report

## Future Work

- More Error Handling
- More Camera Support
- More data aquisition
- Better Data Augmentation
- Different approaches for Fault estimation