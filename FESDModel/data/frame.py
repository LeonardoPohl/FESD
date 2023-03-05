from dataclasses import dataclass
import numpy as np
from pathlib import Path
import cv2

FIGURES_DIR = Path('../docs/Thesis/figures/')

@dataclass
class Frame:
    rgb: np.ndarray
    depth: np.ndarray
    pose_2d: np.ndarray
    poses_3d: np.ndarray
    errors: np.ndarray
    
    def show(self):
      depth_norm = self.depth / 5
      depth_im = np.dstack((depth_norm, depth_norm, depth_norm)) 
      im = np.dstack((self.rgb, depth_im))
      print(im.shape)
      im.convertTo(im, CV_8UC3, 255.0)
      cv2.imshow('depth', im)
      cv2.waitKey()
      cv2.destroyAllWindows()

    def save(self, name: str='GaussianBlur.png', path: Path=FIGURES_DIR / 'Model' / 'Augmentation'):
      depth_norm = self.depth / 5
      depth_im = np.dstack((depth_norm, depth_norm, depth_norm)) 
      im = np.hstack((self.rgb, depth_im))

      cv2.imwrite(str(path / 'GaussianBlur.png'), 255*im)


    
