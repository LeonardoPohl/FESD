import torch
from .mode import Mode

def err2gt(err, mode:Mode=Mode.JOINTS):
  gt = torch.zeros(mode.get_num_layers(), dtype=torch.float32)
  if mode == Mode.JOINTS:
    for (i, e) in enumerate(err):
      gt[int(i*4 + e)] = 1
  
  else:
    for i in range(0, len(gt), 2):
      if err[i//2] == 0:
        gt[i] = 1
      else:
        gt[i+1] = 1
  
  return gt

def errs2gts(errs, mode:Mode=Mode.JOINTS):
  num_error_labels = mode.get_num_error_label()

  gts = torch.zeros(len(errs), len(errs[0]) * num_error_labels, dtype=torch.float32)
  for (i, err) in enumerate(errs):
    gts[i] = err2gt(err, mode)
  return gts

def gt2err(gt, mode:Mode=Mode.JOINTS):
  num_error_labels = mode.get_num_error_label()
    
  err = torch.zeros(len(gt) // num_error_labels, dtype=torch.float32)
  confidence = torch.zeros(len(gt) // num_error_labels, dtype=torch.float32)

  for i in range(0, len(gt), num_error_labels):
    e = gt[i:i + num_error_labels]
    probs = torch.nn.functional.softmax(e, dim=0) 
    conf, classes = torch.max(probs, 0) 
    
    confidence[i // num_error_labels] = conf
    err[i // num_error_labels] = classes

  return err, confidence

def gts2errs(gts, mode:Mode=Mode.JOINTS):
  num_error_labels = mode.get_num_error_label()

  errs = torch.zeros(len(gts), len(gts[0]) // num_error_labels, dtype=torch.float32)
  confidences = torch.zeros(len(gts), len(gts[0]) // num_error_labels, dtype=torch.float32)
  for (i, gt) in enumerate(gts):
    errs[i], confidences[i] = gt2err(gt, mode)
  
  return errs, confidences
