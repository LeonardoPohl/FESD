from utils import AvgMeter, clip_gradient
import torch
import numpy as np

from .eval import val

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs, writer, is_cuda, df):
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

        loss = 0
        
        for j in range(0, 20, 4):
          g = gt[:,j:j+4]
          p = pred[:,j:j+4]
        
          loss += criterion(g, p)
        
        # TODO: model? is this correct it feels like here is something missing
        loss.backward()

        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)
        
        val(pred, gt, writer, loss_record, loss.data, epoch, epochs, i, len(train_loader), "train", session['Session Parameters']["Exercise"][0], df)