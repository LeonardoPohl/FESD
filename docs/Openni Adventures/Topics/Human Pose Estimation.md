[[Papers/Collections/Human Pose Estimation]]
There are different ways to determine the human pose. In this scope we utilise the combined pointcloud of multiple cameras and will therefore be using a point cloud approach to HPE.  [[A_Review_Point_Cloud-Based_3D_Human_Joints_Estimat.pdf]]  Shows various methods on how to do just that. 

*Maybe extract the papers from the review instead of using the review paper itself*

## Machine Learning approach

Nowadays machine learning is everywhere. It can make, bla bla, general solution to specific problem bla bla

Our code base is in C++ which is natively not optimal for machine learning. Therefore we develop a model in python using recorded data and then import the trained model into C++ using [ONNX](https://onnx.ai/)