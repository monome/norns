
# quick and dirty process CPU sampling 
# uses `top` in batch mode, so it is susceptible to usual caveats:

import subprocess
import sys

def run_proc(cmd, args):
    proc = subprocess.Popen(['pidof','crone'],stdout=subprocess.PIPE)
    return proc.stdout.read()[0:-1]

def run_top(pid, n, d):
    args = ['-b', '-n {}'.format(n), '-d {}'.format(d), '-p {}'.format(pid)]
    proc = subprocess.Popen(['top']+args, stdout=subprocess.PIPE)
    txt = proc.stdout.read()
    cpu = []
    for line in txt.split('\n'):
        tok = line.split()
        if len(tok) < 9: continue
        if tok[0] == pid:
            cpu.append(float(tok[8]))
    return cpu


if __name__ == '__main__':
    procname = 'crone'
    n = 10
    d = 0.2
    if len(sys.argv) > 1:
        procname = sys.argv[1]
    if len(sys.argv) > 2:
        n = int(sys.argv[2])
    if len(sys.argv) > 3:
        d = float(sys.argv[3])
            
    pid = run_proc('pidof', procname)
    cpu = run_top(pid, n, d)
    for samp in cpu: print(samp)