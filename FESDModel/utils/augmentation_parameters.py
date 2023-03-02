from dataclasses import dataclass

@dataclass
class AugmentationParams:
    flip: bool = False
    crop: bool = False
    crop_random: bool = False
    crop_pad: int = 0
    gaussian: bool = False
    seed: int = -1