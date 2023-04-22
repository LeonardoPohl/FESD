from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from data import gts2errs
from torchmetrics.classification import BinaryCohenKappa

# eval
def val(prediction, gt, writer, loss_record, epoch, i, data_size, identifier, df):
    gt_err, _ = gts2errs(gt)
    pred_err, pred_confidence = gts2errs(prediction)

    writer.add_scalar('Loss/train', loss_record.show(), i)

    if i == 1 or i == data_size:
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

    df.loc[len(df)] = [epoch, i, gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), loss_record.show(), accuracy, tp, tn, fp, fn,  precision, recall, f1, np.NaN, identifier, False]

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

    df.loc[len(df)] = [epoch, i, gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), loss_record.show(), accuracy, tp, tn, fp, fn,  precision, recall, f1, metric(pred_err, gt_err), identifier, True]

    if i % 100 == 0 or i == data_size:
        print(f'Epoch [{epoch:03d}/{epochs:03d}], \
                Step [{i:04d}/{data_size:04d}], \
                Loss: {loss_record.show():.4f}')
      