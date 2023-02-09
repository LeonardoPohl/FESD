import os

class Joint:
  def __init__(self, joint):
    self.type = joint["i"]
    self.proj = {
      "u": joint["u"],
      "v": joint["v"]
    }
    self.real = {
      "x": joint["x"],
      "y": joint["y"],
      "z": joint["z"]
    }
    self.confidence = joint["score"]

class Skeleton:
  def __init__(self, path):
    self.path = path
    with open(file=path, mode='r') as file:
      data = json.load(file)
      self.frames = []
      for skeleton in data["Skeletons"]:
        for person in skeleton:
          self.joints = []
          for joint in person:
            self.joints.append(Joint(joint))
      
			# joint_json["i"] = joint.type;
			
			# joint_json["u"] = joint.proj.x;
			# joint_json["v"] = joint.proj.y;
			
			# joint_json["x"] = joint.proj.x;
			# joint_json["y"] = joint.proj.y;
			# joint_json["z"] = joint.proj.z;
			
			# joint_json["score"] = joint.confidence;
      