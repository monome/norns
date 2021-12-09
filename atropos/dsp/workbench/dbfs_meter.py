# dbFS metering scale following IEC-60268-18

from scipy import interpolate
import numpy as np
import matplotlib.pyplot as plt

# this dictionary associates dbFS values with deflection percentage
db_deflect = {
    0: 100,
    -10: 75,
    -20: 50,
    -30: 30,
    -40: 15,
    -50: 7.5,
    -60: 5
}

def dbamp(db):
    return pow(10, db * 0.05)

# we want to take linear amplitude and return deflection.
# we'll take the IEC table, convert dbFS to amp, and interpolate equal points.

db = list(db_deflect.keys())
db.sort()
pos = list(map(lambda x: db_deflect[x], db))
amp = list(map(lambda x: dbamp(x), db))

# we want zero amp == zero pos.
# insert an additional point for interpolation source.
amp.insert(0, 0)
pos.insert(0, 0)

interpf  = interpolate.interp1d(amp, pos)
n = 32 # table doesn't have to be huge.
x = np.linspace(0, 1, n)
y = interpf(x)

plt.plot(x, y, '-o')
plt.grid(True)
plt.show()

# export the table, scaled to unit range
print("const float amp_meter_table[] = { ")
print("\t", end='')
for p in y:
    print('{}, '.format(p*0.01), end='')
print("\n}")