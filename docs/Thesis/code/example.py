from distributed_execution import DistributedExecution

def square(v: int) -> int:
    return v * v

values = list(range(1, 10001))

with DistributedExecution() as d:
    results = d.map(square, values, chunk_size=100)
