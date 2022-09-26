import os
import zipfile

def get_if_exists(fname):
    if not os.path.exists(fname):
        return None
    with open(fname, 'r') as f:
        s = f.read()
    return s

def get_multiple(fnames):
    # The valid file should be in one of those paths:
    contents = [get_if_exists(fname) for fname in fnames]
    # Filter invalid / non-existant contents
    filtered = [content for content in contents if content is not None and 'done' in content]
    # We should have at least one file with valid contents.
    # If multiple such files were found, they should all be identical:
    assert len(set(filtered)) == 1
    return filtered[0]

def get_machine(machine_code):
    # Check in the three available output folders:
    dir1 = 'bb0/'
    dir2 = 'bb1/'
    dir3 = 'bb2/'
    return get_multiple([dir1 + machine_code + '.txt',
                         dir2 + machine_code + '.txt',
                         dir3 + machine_code + '.txt'])

def machine_code_to_bytes(m):
    # translate integer-encoded machine code to byte-encoded, like what the
    # original go code outputs:
    extractors = [lambda x: (x >> 3) & 0b1, lambda x: (x >> 4) & 0b1, lambda x: ((x & 0b111) + 1) % 6]
    return bytes([extractors[j](m >> (5 * i)) for i in range(10) for j in range(3)])

def get_undecided_machines(machine_code):
    # obtain the results from a single output file and yields the undecided
    # machines from it
    file_contents = get_machine(machine_code)
    # ignore the final 'done' line
    for line in file_contents.splitlines()[:-1]:
        # Return code 2 from the simulation function denotes undecided machines
        if line.split()[-1] == '2':
            yield int(line.split()[0])

def fix_machine_bytes(machine):
    # due to a typo in the C++ code, the first transition from E is initialized
    # to (0, 1, H) instead of (0, 0, H). This fixes it in the byte encoding:
    if machine[-6:-3] == b'\x01\x00\x00':
        machine = machine[:-6] + b'\x00\x00\x00' + machine[-3:]
    return machine

# read the file index for my results:
with open('starts.txt', 'r') as f:
    # also add the root machine:
    machines = ['28337890523255977'] + f.read().splitlines()

# read and parse the undecided machines from the files with my results:
s = []
for i in range(len(machines)):
    if not i % 100:
        print (i, '/', len(machines))
    s += [fix_machine_bytes(machine_code_to_bytes(machine_code)) for machine_code in get_undecided_machines(machines[i])]
# sort for order-independent comparison
print("S SORTING")
s = sorted(s)

print("S DONE")

# read the file for the original results, from within the zip file:
with zipfile.ZipFile('all_5_states_undecided_machines_with_global_header.zip') as zf:
    with zf.open('all_5_states_undecided_machines_with_global_header') as f:
        header = f.read(30)
        original_seed_db = f.read()

# read the machine bytes for each undecided machine
s2 = []
for i in range(len(original_seed_db) // 30):
    if not i % 1000000:
        print (i, '/', len(original_seed_db) / 30)
    s2.append(original_seed_db[30*i:30*i+30])
# sort for order-independent comparison
print("S2 SORTING")
s2 = sorted(s2)

print("S2 DONE")
# make sure the resulting files are indeed equal:
assert s == s2
