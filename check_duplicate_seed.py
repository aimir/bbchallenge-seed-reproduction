from os import scandir
from hashlib import sha1
from struct import pack

OUTPUT_FOLDER = 'outputs'
UNDECIDED_SPACE_RETURN_VALUE = 2
UNDECIDED_TIME_RETURN_VALUE = 3
EXPECTED_HASH = 'e57063afefd900fa629cfefb40731fd083d90b5e'

def machine_code_to_bytes(m):
    # translate integer-encoded machine code to byte-encoded, like what the
    # original go code outputs:
    extractors = [lambda x: (x >> 3) & 0b1,
                  lambda x: (x >> 4) & 0b1,
                  lambda x: ((x & 0b111) + 1) % 6]
    # for each state, write the written bit, then the tape movement direction,
    # then the new state
    return bytes([extractors[j](m >> (5 * i))
                  for i in range(10)
                  for j in range(3)])

def check_output():
    undecided_space = []
    undecided_time = []
    for entry in scandir(OUTPUT_FOLDER):
        with open(entry.path, 'r') as f:
            data = f.read()
        for row in data.splitlines()[:-1]:
            x, y = map(int, row.split())
            if y == UNDECIDED_SPACE_RETURN_VALUE:
                undecided_space.append(machine_code_to_bytes(x))
            elif y == UNDECIDED_TIME_RETURN_VALUE:
                undecided_time.append(machine_code_to_bytes(x))
    # the two lists are sorted separately
    undecided_space.sort()
    undecided_time.sort()
    # the original contents start with this 30-byte header:
    header = pack('>3IB',
                  len(undecided_time),
                  len(undecided_space),
                  len(undecided_time) + len(undecided_space),
                  1).ljust(30, b'\0')
    output_hash = sha1(header)
    # followed by the sorted encodings of undecided_time machines:
    for machine in undecided_time:
        output_hash.update(machine)
    # and finally, the undecided_space machines:
    for machine in undecided_space:
        output_hash.update(machine)
    # make sure we get the same hash as in the original repository
    assert output_hash.hexdigest() == EXPECTED_HASH

if __name__ == "__main__":
    check_output()
