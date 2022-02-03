import requests
import subprocess
import os

community_url = "https://raw.githubusercontent.com/monome/norns-community/main/community.json"

blacklist = ['internorns', 'amen', 'norns.online', 'nc01-drone']

def run_cmd(cmd, location):
    print(cmd, location)
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, cwd=location)
    output, error = process.communicate()
    return(output, error)

def clone_all(url, location):
    res = requests.get(url)
    cat = res.json()
    for entry in cat['entries']:
        proj_url = entry['project_url']
        name = os.path.basename(proj_url)
        name = os.path.splitext(name)[0]
        if name in blacklist:
            continue
        
        if os.path.exists(os.path.join(location, name)):
            print('repo exists; updating...')
            cmd = 'git pull'
            res = run_cmd(cmd, os.path.join(location, name))
            print(res)
            cmd = 'git submodule update --init --recursive'
            res = run_cmd(cmd, os.path.join(location, name))
            print(res)
        else:                
            res = run_cmd(f'git clone {proj_url} {name}', location)
            print(res)        
            cmd = 'git submodule update --init --recursive'
            res = run_cmd(cmd, os.path.join(location, name))
            print(res)

home = os.path.expanduser("~")
location = os.path.join(home, "norns-all-scripts")
os.mkdir(location)
clone_all(community_url, location)
