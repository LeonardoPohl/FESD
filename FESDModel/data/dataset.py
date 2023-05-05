import json
import os
import torch
import torch.utils.data as data
import torchvision.transforms as transforms
import numpy as np
from pathlib import Path
from enum import Enum

from PIL import Image, ImageOps

from utils.mode import Mode
from utils import gt2err, err2gt

from .frame_loader import load_frame
from .augmentation_parameters import AugmentationParams 
from .frame import Frame

  
class FESDDataset(data.Dataset):
  def __init__(self, recording_dir, im_size:int, test_exercises:list, test:bool=False, mode:Mode=Mode.FULL_BODY, randomize_augmentation_params:bool=False, use_v2:bool=False):
    self.im_size = im_size
    self.recording_dir = recording_dir
    self.recording_jsons = []

    self.size = 0
    self.frames_per_session = 0
    self.total_frames_per_session = 0
    for file in os.listdir(recording_dir):
      if file.endswith('.json'):
        with open(file=os.path.join(recording_dir, file), mode='r') as file:
          data = json.load(file)
          
          self.total_frames_per_session = data['Frames']
          if ((test and data['Session Parameters']['Exercise'] not in test_exercises) or
          (not test and data['Session Parameters']['Exercise'] in test_exercises)):
            continue

          self.size += data['Frames']
          self.frames_per_session = data['Frames']
          self.recording_jsons.append(data)

    print(f"Recordings Found: {len(self.recording_jsons)}")
    print(f"Total Frames: {self.size}")

    self.save_frames = False
    self.use_v2 = use_v2
    self.mode = mode
    self.augmentation_params = AugmentationParams(crop_random=False, crop_pad=0, gaussian=False)
    self.randomize_augmentation_params = randomize_augmentation_params

  def reset_augmentation_params(self):
    self.randomize_augmentation_params = False
    with self.augmentation_params as params:
      params.flip = False
      params.crop = False
      params.crop_random = False
      params.crop_pad = 0
      params.gaussian = False

  def __getitem__(self, index):    
    session, index = self.get_index(index)
    
    if self.randomize_augmentation_params:
      self.augmentation_params.Randomize()
      
    self.frame = load_frame(recording_dir=self.recording_dir, session=self.recording_jsons[session], frame_id=index, im_size=self.im_size, params=self.augmentation_params, mode=self.mode, use_v2=self.use_v2)

    if self.use_v2:
      return self.get_frame_v2(index)
    else:
      return self.get_frame_v1()
      
  def get_frame_v1(self):
    rgb = torch.tensor(self.frame.rgb.copy(), dtype=torch.float32)

    depth = torch.tensor(self.frame.depth.copy(), dtype=torch.float32)
    depth.unsqueeze(0)
    
    if self.augmentation_params.gaussian:
      blurrer = transforms.GaussianBlur(kernel_size=(5, 9), sigma=(0.1, 5))
      rgb = blurrer(rgb)
      depth = blurrer(depth)
    
    pose_2d = torch.tensor(self.frame.pose_2d.copy(), dtype=torch.float32).permute(1, 0)

    errors = torch.tensor(self.frame.errors, dtype=torch.float32)
    
    gt = err2gt(errors, self.mode)
    
    return rgb, depth, pose_2d, gt, self.frame.session

  
  def get_frame_v2(self, index):
    rgb = torch.tensor(self.frame.rgb.copy(), dtype=torch.float32)
    print(rgb.shape)
    rgb_im = Image.fromarray((rgb.numpy() * 255).astype(np.uint8))
    r,g,b = rgb_im.split()
    rgb_im = Image.merge("RGB", (b, g, r))
    
    depth = torch.tensor(self.frame.depth.copy(), dtype=torch.float32)
    depth.unsqueeze(0)
    depth_im = Image.fromarray(((depth.numpy() / 3.5) * 255).astype(np.uint8).repeat(3, axis=2))

    pose = torch.tensor(self.frame.pose_im.copy(), dtype=torch.float32)
    pose.unsqueeze(0)
    pose_im = Image.fromarray((pose.numpy()).astype(np.uint8).repeat(3, axis=2))

    if self.augmentation_params.gaussian:
      blurrer = transforms.GaussianBlur(kernel_size=(5, 9), sigma=(0.1, 5))
      rgb = blurrer(rgb)
      depth = blurrer(depth)
    
    pose_2d = torch.tensor(self.frame.pose_2d.copy(), dtype=torch.float32).permute(1, 0)

    errors = torch.tensor(self.frame.errors, dtype=torch.float32)
    gt = err2gt(errors, self.mode)
    
    merged_image = Image.merge("RGB", (ImageOps.grayscale(image=rgb_im).split()[0], depth_im.split()[0], pose_im.split()[0]))
    
    if self.save_frames:  
      rgb_im.show()
      depth_im.show()
      pose_im.show()
      merged_image.show()

    return merged_image, gt, self.frame.session

  def get_index(self, index):
    session = index // self.frames_per_session
    index = index % self.frames_per_session
    return session, index

  def __len__(self):
    return self.size
