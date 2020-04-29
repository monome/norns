import math
import numpy as np
from scipy import interpolate
import matplotlib.pyplot as plt


def dbamp(db):
    return math.pow(10, (db / 20))

def ampdb(amp):
    return math.log10(amp)* 20

#
# pos_amp = [
#     (100, dbamp(0)),
#     (75, dbamp(-10)),
#     (50, dbamp(-20)),
#     (30, dbamp(-30)),
#     (15, dbamp(-40)),
#     (7.5, dbamp(-50)),
#     (5, dbamp(-60)),
#     (0, 0)
# ]
#
# [pos, amp] = list(zip(*pos_amp))

# hm...
pos = [0, 5, 7.5, 15, 30, 50, 75, 100]
amp = [0, 0.001, 0.0031622776601683794, 0.01, 0.03162277660168379, 0.1, 0.31622776601683794, 1.0]
print(pos)
print(amp)

n = 1024
x = np.append(np.arange(0, 100, 100 / n), 100)

#tck = interpolate.splrep(pos, amp)
#y = interpolate.splev(x, tck, der=0)

# closest.. still a weird little bump in the deltas,
# but i think its fine
y = interpolate.pchip_interpolate(pos, amp, x)

plt.figure()
plt.plot(pos, amp)
plt.plot(x, y)
plt.show()

deltas = list()
for i in range(1, n-1):
    #print((i, y[i]))
    deltas.append(y[i+1] / y[i])

plt.figure()
plt.plot(deltas)
plt.show()
#print(np.max(deltas))
f = open("audio_taper_data.inc", "w")
if True:
    #print("const float audio_taper_table[] = { ")
    for p in y:
        f.write('{}, '.format(p))
    #print("\n}")
f.close()
