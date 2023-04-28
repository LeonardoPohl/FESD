from utils import AvgMeter, clip_gradient
import torch
import numpy as np
from utils import gts2errs
from torchmetrics.classification import BinaryCohenKappa
from tqdm import tqdm

from utils.mode import Mode

# eval
def val(prediction, gt, loss_record, loss, lr, epoch, epochs, i, data_size, identifier, exercise, mode, df):
    gt_errs, _ = gts2errs(gt, mode)
    pred_errs, pred_confidences = gts2errs(prediction, mode)
    
    if mode == Mode.FULL_BODY:
        joint_count = 1
    elif mode == Mode.HALF_BODY:
        joint_count = 2
    elif mode == Mode.LIMBS:
        joint_count = 6
    if mode == Mode.JOINTS:
        joint_count = 20

    for joint_i in range(joint_count):
        gt_err = gt_errs[:,joint_i]
        pred_err = pred_errs[:,joint_i]
        
        if mode == Mode.FULL_BODY:
            pred_confidence = pred_confidences[0]
        else:
            pred_confidence = pred_confidences[joint_i]
        tp = torch.sum(gt_err == pred_err)
        tn = torch.sum(gt_err == pred_err)
        fp = torch.sum(gt_err != pred_err)
        fn = torch.sum(gt_err != pred_err)
        
        precision = tp / (tp + fp)
        recall = tp / (tp + fn)
        
        f1 = (precision * recall) / (precision + recall)

        accuracy = (tp + tn) / (tp + tn + fp + fn)
        metric = BinaryCohenKappa()
        
        df.loc[len(df)] = [epoch, i, joint_i, 
                        gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), 
                        float(loss_record.show()), float(loss), float(accuracy), 
                        float(tp), float(tn), float(fp), float(fn),  float(precision), float(recall), float(f1),
                        np.NaN, lr, identifier, 
                        exercise, False, mode]

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

        df.loc[len(df)] = [epoch, i, joint_i, gt_err.tolist(), pred_err.tolist(), pred_confidence.tolist(), float(loss_record.show()), float(loss), float(accuracy), float(tp), float(tn), float(fp), float(fn),  float(precision), float(recall), float(f1), float(metric(pred_err, gt_err)), lr, identifier, exercise, True, mode]

        #if i % 100 == 0 or i == data_size:
        #    tqdm.write(f'Epoch [{epoch:03d}/{epochs:03d}], \
        #    Step [{i:04d}/{data_size:04d}], \
        #    Loss: {loss_record.show():.4f}')
      