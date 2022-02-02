import os
import glob
import subprocess
import time

def start(executable_file):
    proc = subprocess.Popen(
        executable_file,
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

def capture_output(proc, timeout=1):
    output = []
    start = time.time()
    while True:
        line = readline(proc)
        if (line is not None) and (line != ""):
            #print(line)
            output.append(line)
        if time.time() - start > timeout:
            break
    return output
        
home = os.path.expanduser("~")
exe = os.path.join(home, 'norns/build/matron/matron')
code = os.path.join(home, 'dust/code')

proc = start(exe)
output = capture_output(proc, 3)
for line in output: print(line)

lua = glob.glob(f'{code}/**/*.lua', recursive=True)
scripts = list(filter(lambda p: not '/lib/' in p, lua))


def run_script(path, timeout=1):
    cmd = f"norns.script.load('{path}')"
    print(cmd)
    write(proc, cmd)    
    output = capture_output(proc, timeout)
    for line in output:
        if "!!!!!" in line:
            print(line)


run_script('/home/emb/dust/code/awake/awake.lua')


for script in scripts:
    run_script(script)


os.system("pidof matron | xargs kill")
