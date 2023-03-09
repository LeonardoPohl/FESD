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
  def __init__(self, recording_dir, trainsize, test=False, val=False, train_eval_split=0.8):
    self.trainsize = trainsize
    self.recording_dir = recording_dir
    self.recording_jsons = []
    self.val = val
    self.train_eval_split = train_eval_split

    test_exercises = ['E-1.01', 'E-3.01']
    self.size = 0
    self.frames_per_session = 0
    self.total_frames_per_session = 0
    for file in os.listdir(recording_dir):
      if (file.endswith('.json')):
        with open(file=os.path.join(recording_dir, file), mode='r') as file:
          data = json.load(file)
          self.total_frames_per_session = data['Frames']
          if (test and data['Session Parameters']['Exercise'] not in test_exercises or
          not test and data['Session Parameters']['Exercise'] in test_exercises):
            continue
          
          if test:
            self.size += data['Frames']
            self.frames_per_session = data['Frames']
            self.recording_jsons.append(data)
            continue

          split = int(np.floor(train_eval_split * data['Frames']))
          
          if val:
            self.frames_per_session = data['Frames'] - split
          else:
            self.frames_per_session = split

          self.size += self.frames_per_session
          self.recording_jsons.append(data)

    print(f"Recordings Found: {len(self.recording_jsons)}")
    print(f"Total Frames: {self.size}")

    self.augmentation_params = AugmentationParams(flip=False, crop=False, crop_random=False, crop_pad=0, gaussian=False)
    self.randomize_augmentation_params = False

  def reset_augmentation_params(self):
    self.randomize_augmentation_params = False
    with self.augmentation_params as params:
      params.flip = False
      params.crop = False
      params.crop_random = False
      params.crop_pad = 0
      params.gaussian = False

  def __getitem__(self, index):
    offset = int(np.floor(self.total_frames_per_session * self.train_eval_split if self.val else 0))
    
    session = index // self.frames_per_session
    index = index % self.frames_per_session + offset
    
    if self.randomize_augmentation_params:
      self.augmentation_params.Randomize()

    self.frame = load_frame(recording_dir=self.recording_dir, session=self.recording_jsons[session], frame_id=index, params=self.augmentation_params)
    
    rgb = torch.tensor(self.frame.rgb.copy(), dtype=torch.float32)
    rgb = transforms.Resize(self.trainsize)(rgb.permute(2, 0, 1))
    
    depth = torch.tensor(self.frame.depth.copy(), dtype=torch.float32)
    depth.unsqueeze(0)
    depth = transforms.Resize(self.trainsize)(depth.permute(2, 0, 1))

    if self.augmentation_params.gaussian:
      blurrer = transforms.GaussianBlur(kernel_size=(5, 9), sigma=(0.1, 5))
      rgb = blurrer(rgb)
      depth = blurrer(depth)
    
    pose_2d = torch.tensor(self.frame.pose_2d.copy(), dtype=torch.float32).permute(1, 0)

    errors = torch.tensor(self.frame.errors, dtype=torch.float32)
    gt = self.err2gt(errors)
    
    return rgb, depth, pose_2d, gt

  def err2gt(self, err):
    gt = torch.zeros(len(err) * 4, dtype=torch.float32)
    for (i, e) in enumerate(err):
      gt[int(i*4 + e)] = 1
    return gt
  
  def gt2err(self, gt):
    err = torch.zeros(len(gt) // 4, dtype=torch.float32)
    confidence = torch.zeros(len(gt) // 4, dtype=torch.float32)
    for i in range(0, len(gt), 4):
      e = gt[i:i+4]
      probs = torch.nn.functional.softmax(e, dim=0) 
      conf, classes = torch.max(probs, 0) 
      confidence[i // 4] = conf
      err[i // 4] = classes

    return err, confidence

  def __len__(self):
    return self.size
