import numpy as np
import cv2
import json
from pathlib import Path

from utils.mode import Mode

from .augmentation_parameters import AugmentationParams
from .frame import Frame

def id_2_name(i: int):
  return 'frame_' + str(i) + '.bin'


def load_skeletons(skeletons_json, flip: bool=False, mode=Mode.FULL_BODY) -> (np.ndarray, np.ndarray, np.ndarray, list[tuple[float, float, float]], list[tuple[float, float, float]]):
  bounding_boxes_2d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  bounding_boxes_3d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  
  #i = np.random.randint(0, len(skeletons_json))
  i = 0

  for j, skel in enumerate(skeletons_json):
    if skel["error"] == 0:
      i = j

  person = skeletons_json[i]
  joints_2d = np.ndarray(shape=[0, 3])
  joints_3d = np.ndarray(shape=[0, 3])
  errs = np.ndarray(shape=[0])
  errors = np.ndarray(shape=[0])
  origin = person['Skeleton'][4]

  for joint in person['Skeleton']:
    if (len(person['Skeleton']) == 25) and (joint['i'] in [0, 10, 16, 20, 24]):
      continue
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

    errs = np.append(errs, 2 if person['error'] == 1 else joint['error'])

  upper_body_i = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13]
  lower_body_i = [14, 15, 16, 17, 18, 19]

  torso_i = [2, 3, 4]
  head_i = [0, 1]
  left_arm_i = [5, 6, 7, 8]
  right_arm_i = [10, 11, 12, 13]
  left_leg_i = [14, 15, 16]
  right_leg_i = [17, 18, 19]

  if mode == Mode.FULL_BODY:
    errors = np.append(errors, np.count_nonzero(errs) > 5)
  elif mode == Mode.HALF_BODY:
    errors = np.append(errors, np.count_nonzero(errs[upper_body_i]) > 3)
    errors = np.append(errors, np.count_nonzero(errs[lower_body_i]) > 4)
  elif mode == Mode.LIMBS:
    errors = np.append(errors, np.count_nonzero(errs[torso_i]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[head_i]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[left_arm_i]) > 2)
    errors = np.append(errors, np.count_nonzero(errs[right_arm_i]) > 1)
    errors = np.append(errors, np.count_nonzero(errs[left_leg_i]) > 2)
    errors = np.append(errors, np.count_nonzero(errs[right_leg_i]) > 1)
  elif mode == Mode.JOINTS:
    errors = errs  
  
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

def load_frame(recording_dir: Path, session: json, frame_id: int, params: AugmentationParams = AugmentationParams(), mode: Mode = Mode.FULL_BODY) -> Frame:
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
    pose_2d, pose_3d, errors, bounding_boxes_2d, bounding_boxes_3d = load_skeletons(skeleton_json, params.flip, mode)

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

