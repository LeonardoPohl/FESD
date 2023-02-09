import numpy as np
from src import Recording, get_recordings


if __name__ == "__main__":
  recordings = get_recordings()
  for rec in recordings:
    print(rec.name)