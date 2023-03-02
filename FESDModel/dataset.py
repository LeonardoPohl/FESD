import json
import os
import torch.utils.data as data
import torchvision.transforms as transforms
import numpy as np
from pathlib import Path

from utils import load_frame, AugmentationParams, Frame
  
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

    assert self.size > 0, "No recordings found in {}".format(recording_dir)
    assert self.frames_per_session > 0, "No frames found in {}".format(recording_dir)
    assert self.size / len(self.recording_jsons) == self.frames_per_session, "Frames dont match frames per session {}".format(recording_dir)

    print(f"Recordings Found: {len(self.recording_jsons)}")
    print(f"Total Frames: {self.size}")

    self.img_transform = transforms.Compose([
      transforms.Resize((self.trainsize, self.trainsize)),
      transforms.ToTensor(),
      transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])]) # not sure if this is correct

    self.depths_transform = transforms.Compose([
      transforms.Resize((self.trainsize, self.trainsize)),
      transforms.ToTensor()])

    self.pose_transform = transforms.Compose([
      transforms.ToTensor()])

    self.error_transform = transforms.Compose([
      transforms.ToTensor()])

    self.augmentation_params = AugmentationParams(flip=False, crop=False, crop_random=False, crop_pad=50, gaussian=False)

  def __getitem__(self, index):
    session = index // self.frames_per_session
    index = index % self.frames_per_session
    return load_frame(recording_dir=self.recording_dir, session=self.recording_jsons[session], frame_id=index, params=self.augmentation_params)

  def __len__(self):
    return self.size
