import os
import glob
import subprocess
import time

TIMEOUT_BOOT = 1
TIMEOUT_SCRIPT = 1
WAIT_BOOT = 2
WAIT_SHUTDOWN = 2

def filter_script_paths(paths):
    res = list(filter(lambda p: not '/lib/' in p, paths))
    res = list(filter(lambda p: not 'code/bowering' in p, res))
    res = list(filter(lambda p: not 'code/norns.online' in p, res))
    res = list(filter(lambda p: not 'code/we/' in p, res))
    res = list(filter(lambda p: not 'code/monitor/sequencer' in p, res))
    res = list(filter(lambda p: not 'code/shapes/' in p, res))
    res.sort()
    return res

def start(exe):
    proc = subprocess.Popen(
        exe,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    os.set_blocking(proc.stdout.fileno(), False)
    return proc

def write(proc, message):
    proc.stdin.write(f"{message.strip()}\n".encode("utf-8"))
    proc.stdin.flush()

def readline(proc):
    return proc.stdout.readline().decode("utf-8").strip()

def readline_timeout(proc, timeout=1.0):
    # read a line from a process's stdout
    # (assumed to be in non-blocking mode)
    # return the line read, or None on timeout
    t0 = time.time()
    while True:
        line = readline(proc)
        if (line is not None) and (line != ""):
            return line
        t1 = time.time()
        if (t1-t0) > timeout:
            return None
               
def capture_output(proc, timeout=1, maxtime=8):
    output = []
    start = time.time()
    while True:
        proc.stdout.flush()
        line = readline_timeout(proc, 1)
        print(line)
        if line is not None:
            output.append(line)
            now = time.time()
            if (now-start) > maxtime:
                break
        else:
            break
    return output


def write_script_output(name, output):
    with open(f"output/{name}.txt", "w") as f:
        f.write("\n".join(output))

def run_script(proc, path):
    global TIMEOUT_SCRIPT
    name = os.path.basename(path)
    name = os.path.splitext(name)[0]
           
    print(f'running: {path}')
    cmd = f"norns.script.load('{path}')"
    write(proc, cmd)
    err = False
    paramErr = False
    output = capture_output(proc, TIMEOUT_SCRIPT)
    if len(output) < 1:
        print(" !! no output from script! (process is hung?)")
    write_script_output(name, output)
    ids = []
    for line in output:
        print(line)
        if "!!!!!" in line:
            err = True
            if "parameter id collision" in line:
                id = line.replace("!!!!! error: parameter id collision: ", "")
                ids.append(id.strip())
                paramErr = True
    if err:
        if paramErr:
            print(f"{name}: param ID collision: {', '.join(ids)}")
            
            with open('script_runner.param_err.txt', 'a') as f:
                f.write(f'{path} ({", ".join(ids)})\n')            
                f.close
        else:
            print(f"{name}: other error")
            with open('script_runner.other_err.txt', 'a') as f:
                f.write(f'{path}\n')            
                f.close
    else:
        with open('script_runner.ok.txt', 'a') as f:
            f.write(f'{path}\n')            
            f.close

def rude_shutdown():
    os.system("pidof matron | xargs kill -9")
      
def chunkup(l, n):
    return [l[i:i + n] for i in range(0, len(l), n)]

home = os.path.expanduser("~")
exe = os.path.join(home, 'norns/build/matron/matron')
code = os.path.join(home, 'dust/code')

paths = glob.glob(f'{code}/**/*.lua', recursive=True)
scripts = filter_script_paths(paths)

n = 50
scripts_chunked = chunkup(scripts, n)
print(scripts_chunked)

if True:
    print(f'processing {len(scripts)} scripts...')
    for chunk in scripts_chunked:
        proc = start(exe)    
        time.sleep(WAIT_SHUTDOWN)
        output = capture_output(proc, TIMEOUT_BOOT)
        for path in chunk:
            run_script(proc, path)
            print("\n---------------------------------------------------\n")
        rude_shutdown()
        time.sleep(WAIT_BOOT)