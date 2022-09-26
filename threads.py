from multiprocessing import Pool
from subprocess import Popen, PIPE
import os
from tqdm import tqdm

def run_branch(machine):
    fname = machine + b'.txt'
    if os.path.exists(fname):
        with open(fname, 'r') as f:
            contents = f.read()
        if contents.endswith('done\r\n') or contents.endswith('done\n'):
            return
    proc = Popen(['gen.exe'], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    proc.communicate(machine + b'\r\n')

with open('starts.txt','rb') as f:
    machines = f.read().splitlines()

if __name__ == "__main__":
    with Pool(4) as p:
        value = list(tqdm(p.imap(run_branch, machines), total=len(machines)))
