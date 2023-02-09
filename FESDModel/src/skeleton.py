import os

class Skeleton:
  def __init__(self, path):
    self.path = path
    with open(file=path, mode='r') as file:
      data = json.load(file)
      self.frames = []
      for skeleton in data["Skeletons"]:
        for person in skeleton:
          self.joints = []
      
			# joint_json["i"] = joint.type;
			
			# joint_json["u"] = joint.proj.x;
			# joint_json["v"] = joint.proj.y;
			
			# joint_json["x"] = joint.proj.x;
			# joint_json["y"] = joint.proj.y;
			# joint_json["z"] = joint.proj.z;
			
			# joint_json["score"] = joint.confidence;
      