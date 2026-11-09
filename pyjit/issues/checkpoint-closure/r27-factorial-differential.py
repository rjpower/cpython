import hashlib
import math.integer
import random
rng = random.Random(918273)
values = [0, 1, 2, 3, 12, 13, 20, 21, 22, 30, 63, 64, 127, 128, 255, 256, 511, 512, 999, 1000, 1500]
values += [rng.randrange(1501) for _ in range(80)]
for n in values:
    value = math.integer.factorial(n)
    data = value.to_bytes((value.bit_length() + 7) // 8, 'little')
    print(n, hashlib.sha256(data).hexdigest())
