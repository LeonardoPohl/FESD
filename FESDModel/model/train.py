from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from tqdm import tqdm

from .eval import val

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs, writer, is_cuda, df):
    loss_record = AvgMeter()

    for i, pack in tqdm(enumerate(train_loader, start=1), position=1):
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
        
        loss.backward()
        
        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)
        
        val(pred, gt, loss_record, loss.data, optimizer.param_groups[0]["lr"], epoch, epochs, i, len(train_loader), "train", session['Session Parameters']["Exercise"][0], df)