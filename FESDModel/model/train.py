from utils import AvgMeter, clip_gradient
import torch
from torch.utils.data import DataLoader
import numpy as np
from tqdm import tqdm
from model import FESD, FESDv2

from utils.mode import Mode

from .eval import val

# training
def train(train_loader: DataLoader, model, optimizer, criterion, scheduler, clip, epoch, is_cuda, mode, df, use_v2):
  loss_record = AvgMeter()

  for i, pack in enumerate(train_loader, start=1):
    optimizer.zero_grad()
    
    if use_v2:      
      merged_image, gt, session = pack
      merged_image = merged_image.float()
      
      if is_cuda:
        gt = gt.cuda()
        merged_image = merged_image.cuda()
        
      pred = model(merged_image)
    else:
      rgbs, depths, poses_2d, gt, session = pack
      
      if is_cuda:
        gt = gt.cuda()
        poses_2d = poses_2d.cuda()
        rgbs = rgbs.cuda()
        depths = depths.cuda()

      pred = model(rgbs, depths, poses_2d)

    loss = mode.get_loss(criterion, pred, gt)    
    loss.backward()
    
    clip_gradient(optimizer, clip)
    optimizer.step()
    scheduler.step()
    
    loss_record.update(loss.data, 1)
    
    val(pred, gt, loss_record, loss.data, optimizer.param_groups[0]["lr"], epoch, i, len(train_loader), "train", session['Session Parameters']["Exercise"][0], mode, df, use_v2)
    
  return loss_record.show()