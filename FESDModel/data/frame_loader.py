import numpy as np
import cv2
import json
from pathlib import Path

from utils.mode import Mode

from .augmentation_parameters import AugmentationParams
from .frame import Frame

def pad(mi, ma, pad_mi, pad_ma, min_mi, max_mi, min_ma, max_ma, overflow: bool=True):
  mi -= pad_mi
  if mi < min_mi:
    if overflow:
      ma += min_mi - mi
    mi = min_mi
    
  if mi > max_mi:
    if overflow:
      ma -= mi - max_mi
    mi = max_mi
    
  ma += pad_ma
  if ma < min_ma:
    if overflow:
      mi += min_ma - ma
    ma = min_ma
  
  if ma > max_ma:
    if overflow:
      mi -= ma - max_ma
    ma = max_ma
  return mi, ma
  
def load_skeletons(skeletons_json, flip:bool=False, mode:Mode=Mode.FULL_BODY, use_v2:bool=False) -> (np.ndarray, np.ndarray, np.ndarray, list[tuple[float, float, float]], list[tuple[float, float, float]]):
  bounding_boxes_2d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  bounding_boxes_3d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  
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

    if use_v2:
      joints_2d = np.append(joints_2d, [[
        joint['u'],
        joint['v'],
        joint['d']
        ]], axis=0)

      joints_3d = np.append(joints_3d, [[
        joint['x'],
        joint['y'],
        joint['z']
        ]], axis=0)
    else:
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


  upper_body_i = [0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
  lower_body_i = [3, 14, 15, 16, 17, 18, 19]

  torso_i     = [2, 3, 4, 9]
  head_i      = [0, 1]
  left_arm_i  = [5, 6, 7, 8]
  right_arm_i = [10, 11, 12, 13]
  left_leg_i  = [14, 15, 16]
  right_leg_i = [17, 18, 19]

  if flip:
    # Change left and right body_parts
    errs[left_arm_i], errs[right_arm_i] = errs[right_arm_i], errs[left_arm_i]
    errs[left_leg_i], errs[right_leg_i] = errs[right_leg_i], errs[left_leg_i]

  if mode == Mode.FULL_BODY:
    errors = np.append(errors, np.count_nonzero(errs) > 2)
  elif mode == Mode.HALF_BODY:
    class_dict = mode.get_class_dict()
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Upper Body"]]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Lower Body"]]) > 1)
  elif mode == Mode.BODY_PARTS:
    class_dict = mode.get_class_dict()
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Torso"]]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Head"]]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Left Arm"]]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Right Arm"]]) > 0)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Left Leg"]]) > 1)
    errors = np.append(errors, np.count_nonzero(errs[class_dict["Right Leg"]]) > 1)
  elif mode == Mode.JOINTS:
    errors = errs
  
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

def load_frame(recording_dir: Path, session: json, frame_id: int, params: AugmentationParams = AugmentationParams(), mode: Mode = Mode.FULL_BODY, use_v2:bool=False) -> Frame:
  frame_mat = read_frame(recording_dir /  session['Cameras'][0]['FileName'] / f'frame_{frame_id * 10}.bin')
  frame = np.asarray(frame_mat[:,:])
  rgb, depth = np.split(frame, [3], axis=2)
  rgb, depth = rgb.astype(np.float32), depth.astype(np.float32)
  
  with open(file=recording_dir /  session['Skeleton'], mode='r') as file:
    skeleton_json = json.load(file)[frame_id * 10]
    pose_2d, pose_3d, errors, bounding_boxes_2d, bounding_boxes_3d = load_skeletons(skeleton_json, params.flip, mode, use_v2)
  im_size = int(np.floor(max(abs(bounding_boxes_2d[1][0] - bounding_boxes_2d[0][0]), 
                             abs(bounding_boxes_2d[1][1] - bounding_boxes_2d[0][1])))) + params.crop_pad
  im_size = min(im_size, min(rgb.shape[0], rgb.shape[1]))
  pose_im = np.zeros_like(depth)

  for pose in pose_2d:
    coords = [(int(min(max(0, pose[1] + x), rgb.shape[0] - 1)), int(min(max(0, pose[0] + y), rgb.shape[1] - 1))) for x in range(-3, 4) for y in range(-3, 4)]
    
    for x, y in coords:
      pose_im[x, y] = 255

  rgb, depth, pose_im = crop(rgb, depth, pose_im, im_size, params, bounding_boxes_2d) 

  if params.flip:
    rgb = np.flip(rgb, axis=1)
    depth = np.flip(depth, axis=1)
    pose_im = np.flip(pose_im, axis=1)

  rgb, depth = rgb.astype(dtype=np.float16), depth.astype(dtype=np.float16)

  if rgb.shape[0] != rgb.shape[1]:
    print("WARNING: Image not square!")

  return Frame(rgb, depth, pose_im, pose_2d, pose_3d, errors, session, im_size)


def crop(image_1, image_2, image_3, im_size, params, bounding_boxes_2d):
  min_mi_x, min_mi_y, min_ma_x, min_ma_y = 0, 0, 0, 0
  max_mi_x, max_mi_y, max_ma_x, max_ma_y = image_1.shape[1], image_1.shape[0], image_1.shape[1], image_1.shape[0]

  mi_x = max(int(np.floor(bounding_boxes_2d[0][0])), 0)
  mi_y = max(int(np.floor(bounding_boxes_2d[0][1])), 0)
  min_mi_x, min_mi_y = 0, 0
  max_mi_x, max_mi_y = mi_x, mi_y

  ma_x = min(int(np.floor(bounding_boxes_2d[1][0])), max_ma_x)
  ma_y = min(int(np.floor(bounding_boxes_2d[1][1])), max_ma_y)
  min_ma_x, min_ma_y = ma_x, ma_y
  max_ma_x, max_ma_y = image_1.shape[1], image_1.shape[0]

  if params.crop_pad > 0 or params.crop_random:
    if params.crop_pad > 0:
      mi_x, ma_x = pad(mi_x, ma_x, params.crop_pad, params.crop_pad, min_mi_x, max_mi_x, min_ma_x, max_ma_x, True)
      mi_y, ma_y = pad(mi_y, ma_y, params.crop_pad, params.crop_pad, min_mi_y, max_mi_y, min_ma_y, max_ma_y, True)

    if (params.crop_random):
      if (params.seed == -1):
        seed = np.random.randint(0, 100000)
      else:
        seed = params.seed
      np.random.seed(seed=seed)

      pad_x = im_size - (ma_x - mi_x)
      if pad_x < 0: 
        pad_mi_x = np.random.randint(pad_x, 1)
      elif pad_x > 0:
        pad_mi_x = np.random.randint(0, pad_x + 1)
      else:
        pad_mi_x = 0

      pad_ma_x = pad_mi_x - pad_x

      pad_y = im_size - (ma_x - mi_x)
      if pad_y < 0: 
        pad_mi_y = np.random.randint(pad_y, 1)
      elif pad_x > 0:
        pad_mi_y = np.random.randint(0, pad_y + 1)
      else:
        pad_mi_y = 0

      pad_ma_y = pad_mi_y - pad_y

      mi_x, ma_x = pad(mi_x, ma_x, pad_mi_x, pad_ma_x, min_mi_x, max_mi_x, min_ma_x, max_ma_x, True)
      mi_y, ma_y = pad(mi_y, ma_y, pad_mi_y, pad_ma_y, min_mi_y, max_mi_y, min_ma_y, max_ma_y, True)
    
  if ma_x - mi_x != im_size:
    pad_mi_x = (im_size - (ma_x - mi_x)) // 2
    pad_ma_x = pad_mi_x + (1 if pad_mi_x * 2 + (ma_x - mi_x) != im_size else 0)
    
    mi_x, ma_x = pad(mi_x, ma_x, pad_mi_x, pad_ma_x, min_mi_x, max_mi_x, min_ma_x, max_ma_x, True)
  if ma_y - mi_y != im_size:
    pad_mi_y = (im_size - (ma_y - mi_y)) // 2
    pad_ma_y = pad_mi_y + (1 if pad_mi_y * 2 + (ma_y - mi_y) != im_size else 0)
    
    mi_y, ma_y = pad(mi_y, ma_y, pad_mi_y, pad_ma_y, min_mi_y, max_mi_y, min_ma_y, max_ma_y, True)

  if ma_y - mi_y != ma_x - mi_x: 
    diff_x = (ma_x - mi_x)
    diff_y = (ma_y - mi_y)
    print(f"WARNING: Image is not square (x:{diff_x}, y:{diff_y}) attempting to fix...")

    if diff_y > diff_x:
      diff = diff_y - diff_x
      pad(mi_x, ma_x, diff // 2, diff // 2 + (1 if diff % 2 == 1 else 0), min_mi_x, max_mi_x, min_ma_x, max_ma_x, True)
    else:
      diff = diff_x - diff_y
      pad(mi_y, ma_y, diff // 2, diff // 2 + (1 if diff % 2 == 1 else 0), min_mi_y, max_mi_y, min_ma_y, max_ma_y, True)

    if ma_y - mi_y != ma_x - mi_x:
      raise Exception(f"Failed to fix image (x:{ma_x - mi_x}, y:{ma_y - mi_y})")

  return image_1[mi_y:ma_y, mi_x:ma_x], image_2[mi_y:ma_y, mi_x:ma_x], image_3[mi_y:ma_y, mi_x:ma_x]