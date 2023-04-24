from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from .eval import val

# eval
def test(test_loader, model, criterion, writer, df):
    loss_record = AvgMeter()

    for i, pack in enumerate(test_loader, start=1):      
        rgbs, depths, poses_2d, gt, session = pack
        
        pred = model(rgbs, depths, poses_2d)
        
        loss = 0
        for j in range(0, 20, 4):
          g = gt[:,j:j+4]
          p = pred[:,j:j+4]
        
          loss += criterion(g, p)
        
        loss_record.update(loss.data, 1)
          
        val(pred, gt, writer, loss_record, loss.data, 0, 0, i, len(test_loader), "test", session['Session Parameters']["Exercise"][0], df)