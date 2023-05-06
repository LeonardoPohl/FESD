from dataclasses import dataclass
import numpy as np

@dataclass
class AugmentationParams:
    crop_random: bool = False
    crop_pad: int = 0
    gaussian: bool = False
    flip: bool = False
    seed: int = -1

    def Randomize(self):
        self.crop_random = np.random.choice([True, False])
        self.crop_pad = np.random.randint(0, 100)
        self.gaussian = np.random.choice([True, False])
        self.flip = np.random.choice([True, False])
        self.seed = np.random.randint(0, 100000)