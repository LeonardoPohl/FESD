import torch
import torch.nn as nn
import torch.nn.functional as F
from torchvision.models import EfficientNet_V2_S_Weights, efficientnet_v2_s, EfficientNet

class FESD(nn.Module):
    def __init__(self, num_classes=80):
        super(FESD, self).__init__()

        # RGB Input convolution layers with pooling
        self.rgb_conv1 = nn.Conv2d(3, 32, kernel_size=3, stride=1, padding=1)
        self.rgb_pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.rgb_conv2 = nn.Conv2d(32, 64, kernel_size=3, stride=1, padding=1)
        self.rgb_pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.rgb_conv3 = nn.Conv2d(64, 128, kernel_size=3, stride=1, padding=1)
        self.rgb_pool3 = nn.MaxPool2d(kernel_size=2, stride=2)

        # Depth Input convolution layers with pooling
        self.depth_conv1 = nn.Conv2d(1, 32, kernel_size=3, stride=1, padding=1)
        self.depth_pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.depth_conv2 = nn.Conv2d(32, 64, kernel_size=3, stride=1, padding=1)
        self.depth_pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.depth_conv3 = nn.Conv2d(64, 128, kernel_size=3, stride=1, padding=1)
        self.depth_pool3 = nn.MaxPool2d(kernel_size=2, stride=2)

        # Joint Input convolution layers
        self.joint_conv1 = nn.Conv1d(3, 32, kernel_size=3, stride=1, padding=1)
        self.joint_conv2 = nn.Conv1d(32, 64, kernel_size=3, stride=1, padding=1)
        self.joint_conv3 = nn.Conv1d(64, 128, kernel_size=3, stride=1, padding=1)

        # Fully connected layers
        self.fc1 = nn.Linear(128 * 4 * 4 + 128 * 4 * 4 + 128 * 20, 1024)
        self.fc2 = nn.Linear(1024, 512)
        self.fc3 = nn.Linear(512, num_classes)

        # Dropout layer
        self.dropout = nn.Dropout(p=0.5)

    def forward(self, rgb_image, depth_image, joint_data):
        # RGB Input convolution with pooling
        x = self.rgb_conv1(rgb_image)
        x = nn.functional.relu(x)
        x = self.rgb_pool1(x)
        x = self.rgb_conv2(x)
        x = nn.functional.relu(x)
        x = self.rgb_pool2(x)
        x = self.rgb_conv3(x)
        x = nn.functional.relu(x)
        x = self.rgb_pool3(x)

        # Depth Input convolution with pooling
        y = self.depth_conv1(depth_image)
        y = nn.functional.relu(y)
        y = self.depth_pool1(y)
        y = self.depth_conv2(y)
        y = nn.functional.relu(y)
        y = self.depth_pool2(y)
        y = self.depth_conv3(y)
        y = nn.functional.relu(y)
        y = self.depth_pool3(y)

        # Joint Input convolution
        z = self.joint_conv1(joint_data)
        z = nn.functional.relu(z)
        z = self.joint_conv2(z)
        z = nn.functional.relu(z)
        z = self.joint_conv3(z)
        z = nn.functional.relu(z)

        # Pooling layers for RGB and Depth CNN
        x = nn.functional.max_pool2d(x, kernel_size=2)
        y = nn.functional.max_pool2d(y, kernel_size=2)

        # Flatten and concatenate all three inputs
        x = x.view(-1, x.shape[1] * x.shape[2] * x.shape[3])
        y = y.view(-1, y.shape[1] * y.shape[2] * y.shape[3])
        z = z.view(-1, 128 * 20)

        w = torch.cat([x, y, z], dim=1)

        # Fully connected layers
        w = self.dropout(nn.functional.relu(self.fc1(w)))
        w = self.dropout(nn.functional.relu(self.fc2(w)))
        w = self.fc3(w)

        return w


class FESDv2(nn.Module):
    def __init__(self, num_classes=80):
        super(FESDv2, self).__init__()
        
        weights = EfficientNet_V2_S_Weights(EfficientNet_V2_S_Weights.DEFAULT)
        
        # Inception module for merged input as feature extractor
        self.efficient_net = efficientnet_v2_s(weights=weights)
        self.avgpool = nn.AdaptiveAvgPool2d(1)

        # Fully connected layers
        self.fc1 = nn.Linear(1280, 512)
        self.fc2 = nn.Linear(512, num_classes)

        # Dropout layer
        self.dropout = nn.Dropout(p=0.5)

    def forward(self, merged_image):
        # Not sure if this works already or not
        
        x = self.efficient_net.features(merged_image)
        x = self.avgpool(x)
        w = torch.flatten(x, 1)

        # Fully connected layers
        w = self.dropout(nn.functional.relu(self.fc1(w)))
        w = self.fc2(w)

        return w
