from enum import Enum

class Mode(Enum):
  FULL_BODY = 0
  HALF_BODY = 1
  LIMBS = 2
  JOINTS = 3

  def __eq__(self, o):
    return self.value == o.value

  def get_num_error_label(self):
    if self == Mode.JOINTS:
      return 4
    else:
      return 2
    
  def get_num_joints(self):
    if self == Mode.JOINTS:
      return 20
    elif self == Mode.LIMBS:
      return 6
    elif self == Mode.HALF_BODY:
      return 2
    elif self == Mode.FULL_BODY:
      return 1
    
  def get_num_layers(self):
    return self.get_num_joints() * self.get_num_error_label()

  def get_loss(self, criterion, pred, gt):
    loss = 0

    for j in range(0, self.get_num_layers(), self.get_num_error_label()):
      g = gt[:,j:j+self.get_num_error_label()]
      p = pred[:,j:j+self.get_num_error_label()]
      loss += criterion(g, p)

    return loss

""" err to gt
    if mode == Mode.FULL_BODY:
      # [0, 1]
      #  to
      # [0, 0,
      #  0, 1]
    elif mode == Mode.HALF_BODY:
      # [0, 1, 0, 0]
      #  to
      # [0, 0,
      #  0, 1,
      #  1, 0,
      #  1, 0]
    elif mode == Mode.LIMBS:
      # [0, 1, 0, 0, 1, 1, 0]
      #  to
      # [1, 0, 
      #  0, 1,
      #  1, 0,
      #  1, 0,
      #  0, 1,
      #  0, 1,
      #  1, 0]
    elif mode == Mode.JOINTS:
      # [0, 1, 3, 0, ...]
      #  to
      # [1, 0, 0, 0,
      #  0, 1, 0, 0,
      #  0, 0, 0, 1,
      #  1, 0, 0, 0,
      #  ....]
"""