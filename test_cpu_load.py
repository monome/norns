# quick and dirty process CPU sampling 
# uses `top` in batch mode, so it is susceptible to usual caveats:

import subprocess
import sys
import os

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

def run_jack_cpu(n, d):
    # 'd' is ignored, `jack_cpu_load()` updates every 1s
    cpu = []
    os.environ["PYTHONUNBUFFERED"] = "1"
    proc = subprocess.Popen(['stdbuf', '-i0', '-o0', '-e0', 'jack_cpu_load'], 
                            stdout=subprocess.PIPE, bufsize=1)
    i = 0
    for line in iter(proc.stdout.readline, b''):
        if not line: break
        c = float(line.split()[3])
        #print((i, c))
        sys.stdout.flush()
        cpu.append(c)
        i = i + 1
        if i == n: break
    proc.terminate()
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

    if procname == 'jack':
        cpu = run_jack_cpu(n, d) 
    else:
        pid = run_proc('pidof', procname)
        cpu = run_top(pid, n, d)
    for samp in cpu: print(samp)