class truths:
  def __init__(self, tn, fp, fn, tp):
   self.tp=tp
   self.fp=fp
   self.fn=fn
   self.tn=tn

  def f_beta(self, beta):
    return ((1 + beta**2) * self.precision() * self.recall()) / (beta**2 * self.precision() + self.recall())

  def f05(self):
    return self.f_beta(0.5)

  def f1(self):
    # return self.f_beta(1)
    return 2 * (self.precision() * self.recall()) / (self.precision() + self.recall())
  
  def f2(self):
    return self.f_beta(2)
  
  def precision(self):
    if (self.tp+self.fp) == 0:
      return 0
    return self.tp/(self.tp+self.fp)
  
  def recall(self):
    if (self.tp+self.fn) == 0:
      return 0
    return self.tp/(self.tp+self.fn)
  
  def kappa(self):
    return (2*(self.tp * self.tn - self.fn * self.fp)) / ((self.tp+self.fp)*(self.fp+self.tn)+(self.tp+self.fn)*(self.fn+self.tn)) 
  
  def print(self):
    print("---")
    print(f"  Precision: {self.precision():.2}")
    print(f"  Recall: {self.recall():.2}")
    print(f"  Accuracy: {self.acc():.2}")
    print(f"  F0.5: {self.f05():.2}")
    print(f"  F1: {self.f1():.2}")
    print(f"  F2: {self.f2():.2}")
    print(f"  Kappa: {self.kappa():.2}")
    print("---")
  
  def acc(self):
    # return (self.tp + self.tn)/(self.tp+self.tn+self.fp+self.fn)
    tp = self.tp
    tn = self.tn
    fp = self.fp
    fn = self.fn

    return (tp + tn) / (tp + tn + fp + fn)

print("V1:")
fb_v1 = truths(110, 64, 
                11, 55)
fb_v1.print()


hb_v1 = truths(214, 58, 
               54, 154)
hb_v1.print()
bp_v1 = truths(1039, 219, 
                  99, 83)
bp_v1.print()
# jt_v1 = truths(7992, 338, 648, 55)
# jt_v1.print()

# print("\n\nV2:")

# fb_v2 = truths(169, 5, 66, 0)
# fb_v2.print()
# hb_v2 = truths(214, 58, 54, 154)
# hb_v2.print()
# bp_v2 = truths(1039, 219, 99, 83)
# bp_v2.print()
# jt_v2 = truths(7992, 338, 648, 55)
# jt_v2.print()

