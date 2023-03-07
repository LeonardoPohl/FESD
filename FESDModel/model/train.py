from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from torchviz import make_dot

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs):
    loss_record = AvgMeter()

    mse = np.zeros(100)
    rmse = np.zeros(100)

    for i, pack in enumerate(train_loader, start=1):
        optimizer.zero_grad()
        
        rgbs, depths, poses_2d, errors = pack
        
        rgbs = rgbs.cuda()
        depths = depths.cuda()
        poses = poses_2d.cuda()

        print(rgbs.shape, depths.shape, poses.shape, errors.shape)

        gt = errors.cuda()
        
        pred_s = model(rgbs, depths, poses)
        if (i == 1):
            make_dot(pred_s.mean(), params=dict(list(model.named_parameters()))).render("rnn_torchviz", format="png")

        # TODO Calculate different loss based on the error label
        loss = criterion(pred_s, gt)
        
        loss.backward()
        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)

        mse[i%100] = sum((pred_s - gt)**2) / len(pred_s)
        rmse[i%100] = np.sqrt(mse[i%100])

        if i % 100 == 0 or i == len(train_loader):
          mse_c = torch.tensor(mse).cuda()
          rmse_c = torch.tensor(rmse).cuda()
          print(f'Epoch [{epoch:03d}/{epochs:03d}], \
                  Step [{i:04d}/{len(train_loader):04d}], \
                  Loss: {loss_record.show():.4f}, \
                  MSE: {torch.mean(mse_c):.4f}, \
                  RMSE: {torch.mean(rmse_c):.4f}', end='\r')
      