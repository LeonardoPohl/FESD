import torch.utils.data as data
import torchvision.transforms as transforms
import numpy as np
from dataclasses import dataclass

@dataclass
class Frame:
    rgb: np.ndarray
    depth: np.ndarray
    poses_2d: np.ndarray
    poses_3d: np.ndarray
    errors: np.ndarray

@dataclass
class AugmentationParams:
    flip: bool = False
    crop: bool = False
    crop_random: bool = False
    crop_pad: int = 0
    gaussian: bool = False
    seed: int = -1

  
class FESDDataset(data.Dataset):
  def __init__(self, recording_dir, trainsize):
    self.trainsize = trainsize
    self.recording_jsons = []

    self.size = 0
    self.frames_per_session = 0
    for file in os.listdir(RECORDING_DIR):
      if (file.endswith('.json')):
        with open(file=os.path.join(RECORDING_DIR, file), mode='r') as file:
          data = json.load(file)
          self.size += data['frames']
          frames_per_session = data['frames']
          recording_jsons.append(data)

    assert self.size > 0, "No recordings found in {}".format(RECORDING_DIR)
    assert frames_per_session > 0, "No frames found in {}".format(RECORDING_DIR)
    assert self.size / len(recording_json) == frames_per_session, "Frames dont match frames per session {}".format(RECORDING_DIR)

    print(f"Recordings Found: {len(recording_jsons)}")
    print(f"Total Frames: {total_frames}")

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

    self.augmentation_params = AugmentationParams(flip=False, crop=False, crop_random=False, crop_pad=50, gaussian=True)

  def __getitem__(self, index):
    session = index // self.frames_per_session
    index = index % self.frames_per_session
    return getFrame(session=self.recording_jsons[session], frame_id=index, params=augmentation_params)

  def __len__(self):
    return self.size

# Load Frames(/Videos(Later maybe)) and skeletons from a recording

# TODO: Unsure if this is a problem but bounding boxes will change making crop size different
#     Possible solution: If multiple frames are used to represent a frame, use the same bounding box for all frames
#              For query frames, use the bounding box of the first frame and return list of list of frames as video
# Question: Should the videos be overlapping in frames?
# TODO: Add augmentation maybe?
def load_skeletons(skeletons_json, flip: bool=False) -> (np.ndarray, np.ndarray, list[tuple[float, float, float]], list[tuple[float, float, float]]):
  poses = []
  pose_errors = []
  bounding_boxes_2d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  bounding_boxes_3d = [(np.inf, np.inf, np.inf), (0, 0, 0)]
  for person in skeletons_json:
    joints = np.ndarray(shape=[0, 6])
    errors = []  
    origin = person['Skeleton'][4]

  for joint in person['Skeleton']:
    if (joint['error'] != 1):
      bounding_boxes_2d[0] = np.minimum(bounding_boxes_2d[0], [joint['u'], joint['v'], joint['d']])
      bounding_boxes_2d[1] = np.maximum(bounding_boxes_2d[1], [joint['u'], joint['v'], joint['d']])
      bounding_boxes_3d[0] = np.minimum(bounding_boxes_3d[0], [joint['x'], joint['y'], joint['z']])
      bounding_boxes_3d[1] = np.maximum(bounding_boxes_3d[1], [joint['x'], joint['y'], joint['z']])
      joints = np.append(joints, [[
      joint['u'] - origin['u'],
      joint['v'] - origin['v'],
      joint['d'] - origin['d'],
      joint['x'] - origin['x'],
      joint['y'] - origin['y'],
      joint['z'] - origin['z']
      ]], axis=0) * (-1 if flip else 1)

      errors.append(1 if person['error'] == 1 else joint['error'])
        
  poses.append(joints)
  pose_errors.append(errors)
  
  return np.asarray(poses), np.asarray(pose_errors), bounding_boxes_2d, bounding_boxes_3d

def load_frames(session: json, start: int, stop: int, params: AugmentationParams = AugmentationParams()) -> list[Frame]:
  frames = []

  if (start > stop):
    start, stop = stop, start
  if (start < 0):
    start = 0
  if (stop > session['Frames']):
    stop = session['Frames'] 
  
  params.seed = np.random.randint(0, 100000)
  
  for frame_id in range(start, stop):
    frames.append(load_frame(session, frame_id, params))
  
  return frames

def load_frame(session: json, frame_id: int, params: AugmentationParams = AugmentationParams()) -> Frame:
  frame_path = RECORDING_DIR /  session['Cameras'][0]['FileName'] / id_2_name(frame_id)
  print(frame_path)
  frame_file = cv2.FileStorage(str(frame_path), cv2.FileStorage_READ)
  frame = np.asarray( frame_file.getNode('frame').mat()[:,:] )
  rgb, depth = np.split(frame, [3], axis=2)

  with open(file=RECORDING_DIR /  session['Skeleton'], mode='r') as file:
    skeleton_json = json.load(file)[frame_id]
    poses, errors, bounding_boxes_2d, bounding_boxes_3d = load_skeletons(skeleton_json, params.flip)
    pose_2d, pose_3d = np.split(poses, 2, axis=2)
  
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
    min_x = np.random.randint(0, min_x)
    min_y = np.random.randint(0, min_y)
    max_x = np.random.randint(max_x, rgb.shape[1])
    max_y = np.random.randint(max_y, rgb.shape[0])

  rgb = rgb[min_x:max_x, min_y:max_y]
  depth = depth[min_x:max_x, min_y:max_y] 
  
  if (params.gaussian):
    rgb = cv2.GaussianBlur(rgb, (5, 5), 0)
    depth = cv2.GaussianBlur(depth, (5, 5), 0)

  return Frame(rgb, depth, pose_2d, pose_3d, errors)

