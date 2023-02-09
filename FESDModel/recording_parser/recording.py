import os
import json

from .camera import Camera
from .translation import Translation
from .rotation import Rotation
from .skeleton import Skeleton
from .session import Session

recording_dir = 'H:\Recordings'

def get_recordings():
  recordings = []
  for file in os.listdir(recording_dir):
    if file.endswith('.json') and not file.endswith('Skeleton.json'):
      recordings.append(Recording(os.path.join(recording_dir, file)))
  return recordings

class Recording:
  def __init__(self, path):
    self.path = path
    
    with open(file=path, mode='r') as file:
      data = json.load(file)
      self.name = data['Name']
      self.frames = data['Frames']
      self.duration = data['Duration']

      self.session = Session(data['Session Parameters'])
      
      self.roatation = Rotation(data['Rotation'])
      self.translation = Translation(data['Translation'])

      self.cameras = []
      for cam in data['Cameras']:
        self.cameras.append(Camera(cam))
        