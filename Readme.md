# Laughing Eureka

This repository hopefully contains the last, but successful attempt at implementing my thesis. In this iteration I am using the OpenNi framework to simultaneously stream different cameras. So far it has shown to be quite promising at least for the astra cameras. Even if the Realsense camera does not work with the OpenNi framework I am still hopefull that this might be my last attempt, since the Realsense library is easier to work with than the Astra library.

## Installation

To use the code in this repository some things have to be installed, this might not be a complete list.

- Astra Driver
- Realsense Driver

## Refocus of the thesis

Original plan:
- Build a structure that allows us to find the relative position of multiple cameras
- Align the point clouds
- Find errors in Skeleton detection using multiple angles
- Use these errors to improve the skeleton detection

Proposed plan:
- Build a structure that allows us to find the relative position of multiple cameras
- Find the exact relative position and rotation of each camera
- Calculate the error of the positioning
- Given a preconfigured set-up show what needs to change to be set up correctly
- If using multiple cameras:
	- Overlay the point clouds
	- Calculate the error of the point clouds

I believe that this will still be challenging and very exciting. Such a tool would be very useful