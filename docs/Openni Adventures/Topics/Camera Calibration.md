[[Papers/Collections/Camera Calibration]]

## Inverse Projection

- Camera Image is 2D has to be 3D

https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html
https://towardsdatascience.com/inverse-projection-transformation-c866ccedef1c

## Floor Detection

- Reduce the DOF
	- Makes Camera linking easier
	- Makes it possible to determine general camera positioning, such as camera height pitch and roll

### Plane Detection

- Use RANSAC to find planes?
	- Very slow
- [[An Improved RANSAC for 3D Point Cloud Plane.pdf]]