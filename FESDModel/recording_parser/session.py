class Exercise:
  def __init__(self, exercise):
    self.name = exercise['Name']
    self.difficulty = exercise['Difficulty']
    self.sitting = exercise['Sitting']
    self.ankle_weight = exercise['Ankle Weight']
    self.holding_weight = exercise['Holding Weight']
 
    
class Session:
  def __init__(self, session):
    self.angle = session['Angle']
    self.height = session['Height']
    self.distance = session['Distance']
    
    self.close_background = session['Background close']
    self.cramped = session['Cramped']
    self.dark_clothing = session['Dark Clothing']
    self.exercise = Exercise(session['Exercise'])