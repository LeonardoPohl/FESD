class Camera:
  def __init__(self, camera):
    self.type = camera["Type"]
    self.name = camera["Name"]
    self.file_name = camera["FileName"]
    
    self.cx = camera["Cx"]
    self.cy = camera["Cy"]
    self.fx = camera["Fx"]
    self.fy = camera["Fy"]

    self.meter_per_unit = camera["MeterPerUnit"]
