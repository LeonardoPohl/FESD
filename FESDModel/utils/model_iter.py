from pathlib import Path
import torch
import torch.nn as nn
from torch.utils.data import WeightedRandomSampler
import json

from model import FESD, FESDv2

from .mode import Mode
from .func import AvgMeter, clip_gradient
from .lr_scheduler import get_scheduler


"""
torchrun --nproc_per_node=8 train.py \
--model $MODEL --batch-size 128 --lr 0.5 --lr-scheduler cosineannealinglr \
--lr-warmup-epochs 5 --lr-warmup-method linear --auto-augment ta_wide --epochs 600 --random-erase 0.1 \
--label-smoothing 0.1 --mixup-alpha 0.2 --cutmix-alpha 1.0 --weight-decay 0.00002 --norm-weight-decay 0.0 \
--train-crop-size $TRAIN_SIZE --model-ema --val-crop-size $EVAL_SIZE --val-resize-size $EVAL_SIZE \
--ra-sampler --ra-reps 4
"""

# optimizer
optim = 'adam'
# learning rate
learning_rate = 0.00005
# learning rate scheduler. can be step, poly or cosine
lr_scheduler = 'cosine'
# warmup epoch
warmup_epoch = 0
# warmup multiplier
warmup_multiplier = 100
# for step scheduler. where to decay lr, can be a list
lr_decay_epochs = [120, 160, 200]
# for step scheduler. step size to decay lr
lr_decay_steps = 20
# for step scheduler. decay rate for learning rate
lr_decay_rate = 0.01
# weight decay
weight_decay = 0.0001
# momentum for SGD
momentum = 0.9

RECORDING_DIR = Path('D:/Recordings/')

def get_model_iter(mode: Mode, use_cuda: bool, use_v2: bool, test_exercises: list, epochs: int, batchsize: int, im_size: int, clip: float):
  if use_v2:
    model = nn.DataParallel(FESDv2(mode.get_num_layers()))
  else:
    model = nn.DataParallel(FESD(mode.get_num_layers()))
    
  if use_cuda:
    model.cuda()
  
  from data import FESDDataset
  from data import AugmentationParams
  
  dataset_train = FESDDataset(RECORDING_DIR, im_size, test_exercises=test_exercises, mode=mode, randomize_augmentation_params=True, use_v2=use_v2)
  dataset_test = FESDDataset(RECORDING_DIR, im_size, test_exercises=test_exercises, mode=mode, test=True, use_v2=use_v2)

  with open("weights.json", 'r') as fp:
    weights = json.load(fp)

  problem_set = "Full Body" if mode == Mode.FULL_BODY else "Half Body" if mode == Mode.HALF_BODY else "Body parts" if mode == Mode.BODY_PARTS else "Joints"
  w = weights[problem_set]
  sampler = WeightedRandomSampler(weights=w, num_samples=len(dataset_train))

  train_loader  = torch.utils.data.DataLoader(dataset_train, sampler=sampler, batch_size=batchsize)
  test_loader   = torch.utils.data.DataLoader(dataset_test)

  if optim == 'adam':
    optimiser = torch.optim.Adam(model.parameters(), learning_rate, weight_decay=weight_decay)
  elif optim == 'adamW':
    optimiser = torch.optim.AdamW(model.parameters(), learning_rate, weight_decay=weight_decay)
  elif optim == 'sdg':
    optimiser = torch.optim.SGD(model.parameters(), learning_rate / 10.0 * batchsize, momentum=momentum, weight_decay=weight_decay)

  scheduler = get_scheduler(optimiser, len(train_loader), lr_scheduler, lr_decay_epochs, lr_decay_steps, lr_decay_rate, epochs, warmup_epoch, warmup_multiplier)
  
  return [[mode, model, optimiser, scheduler, train_loader, test_loader]]

def get_model_iter_all(use_cuda: bool, use_v2: bool, test_exercises: list, epochs: int, batchsize: int, im_size: int, clip: float):
  model_iter = []
  
  for mode in [Mode.BODY_PARTS, Mode.HALF_BODY, Mode.FULL_BODY, Mode.JOINTS]:
    model_iter += get_model_iter(mode, use_cuda, use_v2, test_exercises, epochs, batchsize, im_size, clip)

  return model_iter