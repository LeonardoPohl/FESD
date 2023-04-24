from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from data import gts2errs
from torchmetrics.classification import BinaryCohenKappa
from tqdm import tqdm

# eval
def val(prediction, gt, writer, loss_record, loss, lr, epoch, epochs, i, data_size, identifier, exercise, df):
    gt_err, _ = gts2errs(gt)
    pred_err, pred_confidence = gts2errs(prediction)

    tp = torch.sum(gt_err == pred_err)
    tn = torch.sum(gt_err == pred_err)
    fp = torch.sum(gt_err != pred_err)
    fn = torch.sum(gt_err != pred_err)
    
    precision = tp / (tp + fp)
    recall = tp / (tp + fn)
    
    f1 = (precision * recall) / (precision + recall)

    accuracy = (tp + tn) / (tp + tn + fp + fn)
    metric = BinaryCohenKappa()

    df.loc[len(df)] = [epoch, i, gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), float(loss_record.show()), float(loss), float(accuracy), float(tp), float(tn), float(fp), float(fn),  float(precision), float(recall), float(f1), np.NaN, lr, identifier, exercise, False]

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

    df.loc[len(df)] = [epoch, i, gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), float(loss_record.show()), float(loss), float(accuracy), float(tp), float(tn), float(fp), float(fn),  float(precision), float(recall), float(f1), float(metric(pred_err, gt_err)), lr, identifier, exercise, True]

    if i % 100 == 0 or i == data_size:
        tqdm.write(f'Epoch [{epoch:03d}/{epochs:03d}], \
        Step [{i:04d}/{data_size:04d}], \
        Loss: {loss_record.show():.4f}')
      