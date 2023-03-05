import numpy as np
import cv2
import json
from pathlib import Path
from .augmentation_parameters import AugmentationParams
from .frame import Frame

def id_2_name(i: int):
  return 'frame_' + str(i) + '.bin'

# Load Frames(/Videos(Later maybe)) and skeletons from a recording

# TODO: Unsure if this is a problem but bounding boxes will change making crop size different
#     Possible solution: If multiple frames are used to represent a frame, use the same bounding box for all frames
#              For query frames, use the bounding box of the first frame and return list of list of frames as video
# Question: Should the videos be overlapping in frames?
# TODO: Add augmentation maybe?
def load_skeletons(skeletons_json, flip: bool=False) -> (np.ndarray, np.ndarray, np.ndarray, list[tuple[float, float, float]], list[tuple[float, float, float]]):
  bounding_boxes_2d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  bounding_boxes_3d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  
  i = np.random.randint(0, len(skeletons_json))
  
  person = skeletons_json[i]
  joints_2d = np.ndarray(shape=[0, 3])
  joints_3d = np.ndarray(shape=[0, 3])
  errors = np.ndarray(shape=[0])
  origin = person['Skeleton'][4]

  for joint in person['Skeleton']:
    if (joint['error'] != 1):
      bounding_boxes_2d[0] = np.minimum(bounding_boxes_2d[0], [joint['u'], joint['v'], joint['d']])
      bounding_boxes_2d[1] = np.maximum(bounding_boxes_2d[1], [joint['u'], joint['v'], joint['d']])
      bounding_boxes_3d[0] = np.minimum(bounding_boxes_3d[0], [joint['x'], joint['y'], joint['z']])
      bounding_boxes_3d[1] = np.maximum(bounding_boxes_3d[1], [joint['x'], joint['y'], joint['z']])

    joints_2d = np.append(joints_2d, [[
      joint['u'] - origin['u'],
      joint['v'] - origin['v'],
      joint['d'] - origin['d']
      ]], axis=0)

    joints_3d = np.append(joints_3d, [[
      joint['x'] - origin['x'],
      joint['y'] - origin['y'],
      joint['z'] - origin['z']
      ]], axis=0)

    errors = np.append(errors, 1 if person['error'] == 1 else joint['error'])

  joints_2d *= -1 if flip else 1
  joints_3d *= -1 if flip else 1
  
  return joints_2d, joints_3d, errors, bounding_boxes_2d, bounding_boxes_3d

def load_frames(recording_dir: Path, session: json, start: int, stop: int, params: AugmentationParams = AugmentationParams()) -> list[Frame]:
  frames = []

  if (start > stop):
    start, stop = stop, start
  if (start < 0):
    start = 0
  if (stop > session['Frames']):
    stop = session['Frames'] 
  
  params.seed = np.random.randint(0, 100000)
  
  for frame_id in range(start, stop):
    frames.append(load_frame(recording_dir, session, frame_id, params))
  
  return frames

def read_frame(path: Path) -> cv2.Mat:
  with open(path, "rb") as f:
    # Read header
    rows = np.frombuffer(f.read(4), dtype=np.int32)[0]
    cols = np.frombuffer(f.read(4), dtype=np.int32)[0]
    type = np.frombuffer(f.read(4), dtype=np.int32)[0]
    channels = np.frombuffer(f.read(4), dtype=np.int32)[0]

    # Read data
    mat = np.frombuffer(f.read(), dtype=np.float16)
    mat = mat.reshape(rows, cols, channels)

  return mat

def load_frame(recording_dir: Path, session: json, frame_id: int, params: AugmentationParams = AugmentationParams()) -> Frame:
  frame_path = recording_dir /  session['Cameras'][0]['FileName'] / id_2_name(frame_id)
  frame_file = cv2.FileStorage(str(frame_path), cv2.FileStorage_READ)
  frame = np.asarray( frame_file.getNode('frame').mat()[:,:] )
  rgb, depth = np.split(frame, [3], axis=2)
  
  with open(file=recording_dir /  session['Skeleton'], mode='r') as file:
    skeleton_json = json.load(file)[frame_id]
    pose_2d, pose_3d, errors, bounding_boxes_2d, bounding_boxes_3d = load_skeletons(skeleton_json, params.flip)

  if (params.flip):
    rgb = np.flip(rgb, axis=1)
    depth = np.flip(depth, axis=1)

  if params.crop or params.crop_random:
    min_x = max(0, int(np.floor(bounding_boxes_2d[0][1])) - params.crop_pad)
    min_y = max(0, int(np.floor(bounding_boxes_2d[0][0])) - params.crop_pad)

    max_x = min(rgb.shape[1], int(np.ceil(bounding_boxes_2d[1][1])) + params.crop_pad)
    max_y = min(rgb.shape[0], int(np.ceil(bounding_boxes_2d[1][0])) + params.crop_pad)

    if (params.crop_random):
      if (params.seed == -1):
        seed = np.random.randint(0, 100000)
      else:
        seed = params.seed

      np.random.seed(seed)
      min_x = np.random.randint(0, max(0, min_x))
      min_y = np.random.randint(0, max(0, min_y))
      max_x = np.random.randint(min(rgb.shape[1] - 1, max_x), rgb.shape[1])
      max_y = np.random.randint(min(rgb.shape[0] - 1, max_y), rgb.shape[0])

    rgb = rgb[min_x:max_x, min_y:max_y]
    depth = depth[min_x:max_x, min_y:max_y] 
  
  if (params.gaussian):
    rgb = cv2.GaussianBlur(rgb, (5, 5), 0)
    depth = cv2.GaussianBlur(depth, (5, 5), 0)

  return Frame(rgb, depth, pose_2d, pose_3d, errors)

