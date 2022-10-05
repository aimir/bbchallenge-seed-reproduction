from multiprocessing import Pool
from subprocess import Popen, PIPE
from os.path import exists
from sys import argv

def run_branch(machine):
    fname = machine + b'.txt'
    if exists(fname):
        with open(fname, 'r') as f:
            contents = f.read()
        if contents.endswith('done\n') or contents.endswith('done\n'):
            return
    proc = Popen(['./bbchallenge_seed_duplication.out'], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    proc.communicate(machine + b'\n')

if __name__ == "__main__":
    with open('starts.txt','rb') as f:
        machines = f.read().splitlines()
    with Pool(int(argv[1])) as p:
        p.map(run_branch, machines)
