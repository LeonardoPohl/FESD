from utils import AvgMeter, clip_gradient
import torch
import numpy as np

from .eval import val

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs, writer, is_cuda, df):
    loss_record = AvgMeter()

    mse = np.zeros(100)
    rmse = np.zeros(100)

    for i, pack in enumerate(train_loader, start=1):
        optimizer.zero_grad()
        
        rgbs, depths, poses_2d, gt = pack
                
        if is_cuda:
          gt = gt.cuda()
          poses_2d = poses_2d.cuda()
          rgbs = rgbs.cuda()
          depths = depths.cuda()

        pred = model(rgbs, depths, poses_2d)

        loss = 0
        # TODO this is not tested
        for i in range(0, 20, 4):
          g = gt[:,i:i+4]
          p = pred[:,i:i+4]
        
          loss += criterion(g, p)
          
        loss.backward()

        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)
        
        val(pred, gt, writer, loss_record, epoch, i, len(train_loader), "train", df)