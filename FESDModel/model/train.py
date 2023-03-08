from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from torchviz import make_dot

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs, writer):
    loss_record = AvgMeter()

    mse = np.zeros(100)
    rmse = np.zeros(100)

    for i, pack in enumerate(train_loader, start=1):
        optimizer.zero_grad()
        
        rgbs, depths, poses_2d, gt = pack
        
        rgbs = rgbs.cuda()
        depths = depths.cuda()
        poses = poses_2d.cuda()

        gt = gt.cuda()
        
        pred = model(rgbs, depths, poses)
        if (i == 1):
            make_dot(pred.mean(), params=dict(list(model.named_parameters()))).render("rnn_torchviz", format="png")

        loss = criterion(pred, gt)
        
        loss.backward()
        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)

        mse[i%100] = sum(sum((pred - gt)**2) / len(pred)) / rgbs.shape[0]
        rmse[i%100] = np.sqrt(mse[i%100])

        if i == 1 or i == len(train_loader):
          writer.add_graph(model, (rgbs, depths, poses))
          print(pred)
          print(gt)

        writer.add_scalar('Loss/train', loss_record.show(), i)
        writer.add_scalar('RMSE/train', rmse[i%100], i)

        if i % 100 == 0 or i == len(train_loader):
          mse_c = torch.tensor(mse).cuda()
          rmse_c = torch.tensor(rmse).cuda()
          print(f'Epoch [{epoch:03d}/{epochs:03d}], \
                  Step [{i:04d}/{len(train_loader):04d}], \
                  Loss: {loss_record.show():.4f}, \
                  MSE: {torch.mean(mse_c):.4f}, \
                  RMSE: {torch.mean(rmse_c):.4f}')
      