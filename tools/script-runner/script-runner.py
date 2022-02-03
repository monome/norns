import os
import glob
import subprocess
import time

TIMEOUT_BOOT = 4
TIMEOUT_SCRIPT = 2

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
            output.append(line)
        if time.time() - start > timeout:
            break
    return output
        
home = os.path.expanduser("~")
exe = os.path.join(home, 'norns/build/matron/matron')
code = os.path.join(home, 'dust/code')

proc = start(exe)
output = capture_output(proc, TIMEOUT_BOOT)
#for line in output: print(line)

lua = glob.glob(f'{code}/**/*.lua', recursive=True)
scripts = list(filter(lambda p: not '/lib/' in p, lua))

scripts_ok = []
scripts_param_err = []
scripts_other_err = []

SAVE_FULL_ERRORS = True

def save_error_output(name, output):
    with open(f"{name}.output.txt", "w") as f:
        f.write("\n".join(output))

def run_script(path):
    name = os.path.basename(path)
    cmd = f"norns.script.load('{path}')"
    #print(cmd)
    write(proc, cmd)
    err = False
    paramErr = False
    output = capture_output(proc, TIMEOUT_SCRIPT)
    ids = []
    for line in output:
        if "!!!!!" in line:
            #print(line)
            err = True
            if "parameter id collision" in line:
                id = line.replace("!!!!! error: parameter id collision: ", "")
                ids.append(id.strip())
                paramErr = True
    if err:
        if paramErr:
            print(f"{name}: param ID collision: {', '.join(ids)}")
            scripts_param_err.append(path)            
        else:
            print(f"{name}: other error")
            scripts_other_err.append(path)
            if SAVE_FULL_ERRORS:
                save_error_output(name, output)
                
    else:
       scripts_ok.append(path)         


#run_script('/home/emb/dust/code/awake/awake.lua')

print(f'processing {len(scripts)} scripts...')
for script in scripts:
    run_script(script)

if len(scripts_ok) > 0:
    print('these scripts had parameter ID collisions:') 
    for script in scripts_param_err:
        print(f"    {script}")
    print("")


if len(scripts_ok) > 0:
    print('these scripts had other errors:')
    for script in scripts_other_err:
        print(f"    {script}")
    print("")

if len(scripts_ok) > 0:
    print('these scripts were OK:') 
    for script in scripts_ok:
        print(f"    {script}")

os.system("pidof matron | xargs kill -9")
