import json
import os
import torch
import torch.utils.data as data
import torchvision.transforms as transforms
import numpy as np
from pathlib import Path

from .frame_loader import load_frame
from .augmentation_parameters import AugmentationParams 
from .frame import Frame
  
class FESDDataset(data.Dataset):
  def __init__(self, recording_dir, trainsize):
    self.trainsize = trainsize
    self.recording_dir = recording_dir
    self.recording_jsons = []

    self.size = 0
    self.frames_per_session = 0
    for file in os.listdir(recording_dir):
      if (file.endswith('.json')):
        with open(file=os.path.join(recording_dir, file), mode='r') as file:
          data = json.load(file)
          self.size += data['Frames']
          self.frames_per_session = data['Frames']
          self.recording_jsons.append(data)

    print(f"Recordings Found: {len(self.recording_jsons)}")
    print(f"Total Frames: {self.size}")

    self.augmentation_params = AugmentationParams(flip=False, crop=False, crop_random=False, crop_pad=0, gaussian=False)
    self.randomize_augmentation_params = False

  def reset_augmentation_params(self):
    self.randomize_augmentation_params = False
    self.augmentation_params = AugmentationParams(flip=False, crop=False, crop_random=False, crop_pad=0, gaussian=False) 

  def __getitem__(self, index):
    session = index // self.frames_per_session
    index = index % self.frames_per_session
    
    if self.randomize_augmentation_params:
      self.augmentation_params.Randomize()

    self.frame = load_frame(recording_dir=self.recording_dir, session=self.recording_jsons[session], frame_id=index, params=self.augmentation_params)
    
    rgb = torch.tensor(self.frame.rgb.copy(), dtype=torch.float32)
    rgb = transforms.Resize(self.trainsize)(rgb.permute(2, 0, 1))
    
    depth = torch.tensor(self.frame.depth.copy(), dtype=torch.float32)
    depth.unsqueeze(0)
    depth = transforms.Resize(self.trainsize)(depth.permute(2, 0, 1))

    print(depth.shape)
    if self.augmentation_params.gaussian:
      blurrer = transforms.GaussianBlur(kernel_size=(5, 9), sigma=(0.1, 5))
      rgb = blurrer(rgb)
      depth = blurrer(depth)
    print(depth.shape)
    
    pose_2d = torch.tensor(self.frame.pose_2d.copy(), dtype=torch.float32).permute(1, 0)
    errors = torch.tensor(self.frame.errors, dtype=torch.float32)
    
    return rgb, depth, pose_2d, errors

  def __len__(self):
    return self.size
