import os
import json
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
      self.roatation = data['Rotation']
      self.translation = data['Translation']
      self.cameras = data['Cameras']
      self.recordings = []
      for cam in self.cameras:
        self.recordings.append(cam["FileName"])
      self.session = data['Session Parameters']
    