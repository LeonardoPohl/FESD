from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from torchviz import make_dot
from data import errs2gts, gts2errs
from torchmetrics.classification import BinaryCohenKappa

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
        writer.add_scalar('Loss/train', loss_record.show(), i)

        pred_err, pred_conf = gts2errs(pred)
        gt_err, _ = gts2errs(gt)

        if i == 1 or i == len(train_loader):
          writer.add_graph(model, (rgbs, depths, poses))

        tp = torch.sum(gt_err == pred_err)
        tn = torch.sum(gt_err == pred_err)
        fp = torch.sum(gt_err != pred_err)
        fn = torch.sum(gt_err != pred_err)
        
        precision = tp / (tp + fp)
        recall = tp / (tp + fn)
      
        f1 = (precision * recall) / (precision + recall)

        accuracy = (tp + tn) / (tp + tn + fp + fn)
        metric = BinaryCohenKappa()

        writer.add_scalar('accuracy/complete/train', accuracy, i)
        writer.add_scalar('precision/complete/train', precision, i)
        writer.add_scalar('recall/complete/train', recall, i)
        writer.add_scalar('f1/complete/train', f1, i)

        pred_err = pred_err.apply_(lambda x: 0 if x == 0 else 1)
        gt_err = gt_err.apply_(lambda x: 0 if x == 0 else 1)
        
        tp = torch.sum(torch.logical_and(gt_err == 0, pred_err == 0))
        tn = torch.sum(torch.logical_and(gt_err != 0, pred_err != 0))
        fp = torch.sum(torch.logical_and(gt_err != 0, pred_err == 0))
        fn = torch.sum(torch.logical_and(gt_err == 0, pred_err != 0))
        
        precision = torch.sum(pred_err == gt_err) / len(pred_err)
        recall = torch.sum(pred_err == gt_err) / len(gt_err)
        f1 = 2 * (precision * recall) / (precision + recall)

        accuracy = (tp + tn) / (tp + tn + fp + fn)

        writer.add_scalar('accuracy/simplified/train', accuracy, i)
        writer.add_scalar('precision/simplified/train', precision, i)
        writer.add_scalar('recall/simplified/train', recall, i)
        writer.add_scalar('f1/simplified/train', f1, i)
        writer.add_scalar('cohen_kappa/simplified/train', metric(pred_err, gt_err), i)

        if i % 100 == 0 or i == len(train_loader):
          print(f'Epoch [{epoch:03d}/{epochs:03d}], \
                  Step [{i:04d}/{len(train_loader):04d}], \
                  Loss: {loss_record.show():.4f}')
      