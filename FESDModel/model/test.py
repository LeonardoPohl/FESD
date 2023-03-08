from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from torchviz import make_dot

# eval
def test(test_loader, model, criterion, epoch, epochs, writer):
    loss_record = AvgMeter()

    mse = np.zeros(100)
    rmse = np.zeros(100)

    for i, pack in enumerate(eval_loader, start=1):        
        rgbs, depths, poses_2d, errors = pack
        
        rgbs = rgbs.cuda()
        depths = depths.cuda()
        poses = poses_2d.cuda()

        gt = errors.cuda()
        
        pred_s = model(rgbs, depths, poses)
        
        # TODO Calculate different loss based on the error label
        loss = criterion(pred_s, gt)
        loss_record.update(loss)

        mse[i%100] = sum(sum((pred_s - gt)**2) / len(pred_s)) / rgbs.shape[0]
        rmse[i%100] = np.sqrt(mse[i%100])

        writer.add_scalar('Loss/test', loss_record.show(), i)
        writer.add_scalar('MSE/test', sum(sum((pred_s - gt)**2) / len(pred_s)) / rgbs.shape[0], i)
      