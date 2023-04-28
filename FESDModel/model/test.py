from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from .eval import val

# eval
def test(test_loader, model, criterion, writer, is_cuda, mode, df):
    loss_record = AvgMeter()

    for i, pack in enumerate(test_loader, start=1):      
        rgbs, depths, poses_2d, gt, session = pack
        
        if is_cuda:
          gt = gt.cuda()
          poses_2d = poses_2d.cuda()
          rgbs = rgbs.cuda()
          depths = depths.cuda()

        pred = model(rgbs, depths, poses_2d)
        
        loss = mode.get_loss(criterion, pred, gt)
        
        loss_record.update(loss.data, 1)
          
        val(pred, gt, loss_record, loss.data, np.NaN, 0, 0, i, len(test_loader), "test", session['Session Parameters']["Exercise"][0], mode, df)