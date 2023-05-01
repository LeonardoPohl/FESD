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

  def get_class_dict(self):
    if self == Mode.JOINTS or self == Mode.FULL_BODY:
      raise Exception(f"Class not supported for this mode ({self.name})")
    elif self == Mode.LIMBS:
      head_i      = [0, 1]
      torso_i     = [2, 3, 4, 9]
      left_arm_i  = [5, 6, 7, 8]
      right_arm_i = [10, 11, 12, 13]
      left_leg_i  = [14, 15, 16]
      right_leg_i = [17, 18, 19]

      return {"Torso": torso_i,
              "Head": head_i,
              "Left Arm": left_arm_i,
              "Right Arm": right_arm_i,
              "Left Leg": left_leg_i,
              "Right Leg": right_leg_i}
    elif self == Mode.HALF_BODY:
      upper_body_i = [0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
      lower_body_i = [3, 14, 15, 16, 17, 18, 19]
      return {"Upper Body": upper_body_i,
              "Lower Body": lower_body_i}

  def get_class(self, index):
    if self == Mode.JOINTS or self == Mode.FULL_BODY:
      raise Exception(f"Class not supported for this mode ({self.name})")
    elif self == Mode.LIMBS:
      torso_i     = self.get_class_dict()["Torso"]
      head_i      = self.get_class_dict()["Head"]
      left_arm_i  = self.get_class_dict()["Left Arm"]
      right_arm_i = self.get_class_dict()["Right Arm"]
      left_leg_i  = self.get_class_dict()["Left Leg"]
      right_leg_i = self.get_class_dict()["Right Leg"]

      return ("Left Arm"  if index in self.get_class_dict()["Left Arm"]   else 
              "Right Arm" if index in self.get_class_dict()["Right Arm"]  else 
              "Left Leg"  if index in self.get_class_dict()["Left Leg"]   else 
              "Right Leg" if index in self.get_class_dict()["Right Leg"]  else 
              "Torso"     if index in self.get_class_dict()["Torso"]      else 
              "Head"      if index in self.get_class_dict()["Head"]       else "None")
    elif self == Mode.HALF_BODY:
      return ("Upper Body" if index in self.get_class_dict()["Upper Body"] else
              "Lower Body" if index in self.get_class_dict()["Lower Body"] else "None")

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