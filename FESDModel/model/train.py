from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from tqdm import tqdm

from utils.mode import Mode

from .eval import val

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs, is_cuda, mode, df):
  loss_record = AvgMeter()

  for i, pack in enumerate(train_loader, start=1):
    optimizer.zero_grad()
    
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
    
    val(pred, gt, loss_record, loss.data, optimizer.param_groups[0]["lr"], epoch, epochs, i, len(train_loader), "train", session['Session Parameters']["Exercise"][0], mode, df)
    
  return loss_record.show()