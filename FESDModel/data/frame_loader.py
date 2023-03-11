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
  frame_mat = read_frame(recording_dir /  session['Cameras'][0]['FileName'] / id_2_name(frame_id * 10))
  frame = np.asarray( frame_mat[:,:] )
  rgb, depth = np.split(frame, [3], axis=2)
  rgb, depth = rgb.astype(np.float32), depth.astype(np.float32)

  # calculate number of rows/columns to remove
  c_to_remove = rgb.shape[1] - rgb.shape[0]

  if c_to_remove % 2 == 0:
      rgb = rgb[:, c_to_remove//2:-c_to_remove//2, :]
      depth = depth[:, c_to_remove//2:-c_to_remove//2, :]
  else:
      rgb = rgb[:, (c_to_remove-1)//2:-(c_to_remove+1)//2, :]
      depth = depth[:, (c_to_remove-1)//2:-(c_to_remove+1)//2, :]

  with open(file=recording_dir /  session['Skeleton'], mode='r') as file:
    skeleton_json = json.load(file)[frame_id * 10]
    pose_2d, pose_3d, errors, bounding_boxes_2d, bounding_boxes_3d = load_skeletons(skeleton_json, params.flip)

  if (params.flip):
    rgb = np.flip(rgb, axis=1)
    depth = np.flip(depth, axis=1)

  if params.crop or params.crop_random:
    mi = min(int(np.floor(bounding_boxes_2d[0][1])), int(np.floor(bounding_boxes_2d[0][0])))
    mi = max(0, mi - params.crop_pad)

    ma = max(int(np.ceil(bounding_boxes_2d[1][1])), int(np.ceil(bounding_boxes_2d[1][0])))
    ma = min(min(rgb.shape[0], rgb.shape[1]), ma + params.crop_pad)

    if (params.crop_random):
      if (params.seed == -1):
        seed = np.random.randint(0, 100000)
      else:
        seed = params.seed

      np.random.seed(seed)
      mi = np.random.randint(0, max(0, mi))
      ma = np.random.randint(min(rgb.shape[1] - 1, ma), rgb.shape[1])

    rgb = rgb[mi:ma, mi:ma]
    depth = depth[mi:ma, mi:ma] 
  
  if (params.gaussian and False):
    rgb = cv2.GaussianBlur(rgb, (5, 5), 0)
    depth = cv2.GaussianBlur(depth, (5, 5), 0)

  rgb, depth = rgb.astype(dtype=np.float16), depth.astype(dtype=np.float16)

  return Frame(rgb, depth, pose_2d, pose_3d, errors, session)

