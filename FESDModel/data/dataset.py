import json
import os
import torch
import torch.utils.data as data
import torchvision.transforms as transforms
import numpy as np
from pathlib import Path
from enum import Enum

from PIL import Image, ImageOps, ImageFilter

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
    exercises = []
    count = 0
    for file in os.listdir(recording_dir):
      if not file.endswith('.json'):
        continue
      count += 1
      with open(file=os.path.join(recording_dir, file), mode='r') as file:
        data = json.load(file)
        
        self.total_frames_per_session = data['Frames']

        if test:
          if data['Session Parameters']['Exercise'] in test_exercises:
            if data['Session Parameters']['Exercise'] not in exercises:
              exercises.append(data['Session Parameters']['Exercise'])
              continue
            else:
              pass
          else:
            continue
        else:
          if data['Session Parameters']['Exercise'] in test_exercises:
            if data['Session Parameters']['Exercise'] not in exercises:
              exercises.append(data['Session Parameters']['Exercise'])
            else:
              continue
          else:
            pass

        self.size += data['Frames']
        self.frames_per_session = data['Frames']
        self.recording_jsons.append(data)
    
    self.use_v2 = use_v2
    self.mode = mode
    self.augmentation_params = AugmentationParams(crop_random=False, crop_pad=0, gaussian=False)
    self.randomize_augmentation_params = randomize_augmentation_params
    self.to_tensor = True
    self.pil_to_tensor = transforms.Compose([transforms.PILToTensor()])

  def reset_augmentation_params(self):
    self.randomize_augmentation_params = False
    
    self.augmentation_params.flip = False
    self.augmentation_params.crop = False
    self.augmentation_params.crop_random = False
    self.augmentation_params.crop_pad = 0
    self.augmentation_params.gaussian = False

  def __getitem__(self, i):    
    i %= self.size
    session, index = self.get_index(i)
    
    if self.randomize_augmentation_params:
      self.augmentation_params.Randomize()
      
    self.frame = load_frame(recording_dir=self.recording_dir, session=self.recording_jsons[session], frame_id=index, params=self.augmentation_params, mode=self.mode, use_v2=self.use_v2)

    if self.use_v2:
      return self.get_frame_v2()
    else:
      return self.get_frame_v1()
      
  def get_rgb_im(self):
    rgb = torch.tensor(self.frame.rgb.copy(), dtype=torch.float32)
    rgb_im = Image.fromarray((rgb.numpy() * 255).astype(np.uint8))
    r,g,b = rgb_im.split()
    rgb_im = Image.merge("RGB", (b, g, r))
    
    if self.augmentation_params.gaussian:
      rgb_im = rgb_im.filter(ImageFilter.GaussianBlur(3))
    
    return rgb_im

  def get_depth_im(self):
    depth = torch.tensor(self.frame.depth.copy(), dtype=torch.float32)
    depth.unsqueeze(0)
    norm_depth = (depth.numpy() - 1.8) / 1.5
    norm_depth[norm_depth > 1] = 1
    norm_depth[norm_depth < 0] = 0
    depth_im = Image.fromarray((norm_depth * 255).astype(np.uint8).repeat(3, axis=2))

    if self.augmentation_params.gaussian:
      depth_im = depth_im.filter(ImageFilter.GaussianBlur(3))

    return depth_im
    
  def get_frame_v1(self):
    rgb_im = self.get_rgb_im()
    rgb_im = ImageOps.scale(rgb_im, self.im_size/float(self.frame.im_size))
    if self.to_tensor:
      rgb_im = self.pil_to_tensor(rgb_im).float() / 255.

    depth_im = self.get_depth_im()
    depth_im = ImageOps.scale(depth_im, self.im_size/float(self.frame.im_size))
    depth_im = depth_im.split()[0]
    if self.to_tensor:
      depth_im = self.pil_to_tensor(depth_im).float() / 255.

    if self.to_tensor:
      assert depth_im.size()[1:] == rgb_im.size()[1:]  
      if depth_im.size()[1:] != (self.im_size, self.im_size):
        print(f"{depth_im.size()[1:]} != {(self.im_size, self.im_size)}")
        
    pose_2d = torch.tensor(self.frame.pose_2d.copy(), dtype=torch.float32).permute(1, 0)
    
    pose = torch.tensor(self.frame.pose_im.copy(), dtype=torch.float32)
    pose.unsqueeze(0)
    pose_im = Image.fromarray((pose.numpy()).astype(np.uint8).repeat(3, axis=2))
    pose_im = ImageOps.scale(pose_im, self.im_size/float(self.frame.im_size))

    errors = torch.tensor(self.frame.errors, dtype=torch.float32)    
    gt = err2gt(errors, self.mode)
    
    return rgb_im, depth_im, pose_2d, gt, self.frame.session

  
  def get_frame_v2(self):
    rgb_im = self.get_rgb_im()
    depth_im = self.get_depth_im()

    pose = torch.tensor(self.frame.pose_im.copy(), dtype=torch.float32)
    pose.unsqueeze(0)
    pose_im = Image.fromarray((pose.numpy()).astype(np.uint8).repeat(3, axis=2))
    
    errors = torch.tensor(self.frame.errors, dtype=torch.float32)
    gt = err2gt(errors, self.mode)
    
    merged_image = Image.merge("RGB", (ImageOps.grayscale(image=rgb_im).split()[0], depth_im.split()[0], pose_im.split()[0]))
    merged_image = ImageOps.scale(merged_image, self.im_size/float(self.frame.im_size))

    if self.to_tensor:
      merged_image = self.pil_to_tensor(merged_image)

    return merged_image, gt, self.frame.session

  def get_index(self, index):
    session = index // self.frames_per_session
    index = index % self.frames_per_session
    return session, index

  def __len__(self):
    return self.size
