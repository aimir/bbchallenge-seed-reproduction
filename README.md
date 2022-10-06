# BBChallenge Seed Database Duplication

This code is designed to replicate and verify the work done in [bbchallenge-seed](https://github.com/bbchallenge/bbchallenge-seed), to generate a database of undecided 5-state Turing machines.

The results I had agree with those of the original repository completely - it found the exact same `88,664,064` undecided machines as the original repository, as well as the same total number of machines enumerated. To be more accurate, the original repository has `125,479,953` while this one has `125,479,954` - but the difference is due to the fact that this code also counts the first machine `1RB---------------------------` in that number, while the original repository doesn't.

## The Contents of this Repository

### The `bbchallenge_seed_duplication.cc` File

I've written an enumeration, simulation and pruning code in C++. Since simulations very quickly become runtime-bound because of memory access, this code minimizes large memory access:

1. It stores the tape of machines as an array of 64-bit integers instead of bits.
2. It encodes machines (and simulation results) as integers that can fit inside a machine register, instead of a larger object that might require memory access to read or write to.

Details and encoding information can be found in the file itself - I tried to keep it clear and well-documented, even while improving its performance.

### Parallelization and Threading

One of the neat things about this simulation method is that it is very parallelizable. In this case, in order to avoid having to product a single big output file on  a single machine, I first ran the program while only allowing at least 5 halting transitions - that is, changing the condition on line TODO to be `if (((machine >> 50) & 0b1111) > 5)`.

I have then taken each of the resulting `7,322` halting machines that had exactly 5 halting, and written their integer encodings to the file `starts.txt`. I could then iterate on each of those machines separately, generating its subtree in a separate text file. In order to help with that, I used the given `threads.py` file which does this in a multi-threaded way.

Note that this means those `7,322` halting machines actually appear twice in our output, once in the file `28329094430233769.txt` of machines generated from the root, and once as the first machine of their own `<machine_encoding_number>.txt` file. This does not effect the duplication checks in any way, since all of those machines are halting, and are therefore irrelevant to the comparison of the undecided machines.

### Output

I distributed the resulting 7322 smaller tasks were distributed to `48` threads on a single machine. In the interest of transparency, I opted to keep the output files in this structure and not to modify them at all. The results are given as-is inside the `output.zip` file.

In order to compare the final results with the original [all_5_states_undecided_machines_with_global_header.zip](https://dna.hamilton.ie/tsterin/all_5_states_undecided_machines_with_global_header.zip) file, I used the given `check_duplicate_seed.py` script, which converts the undecided machine outputs to the format of the original repository, calculates the resulting hash, and asserts that it is the same as the expected hash of the original data.

## Methodology

### Keeping it Transparent

My topmost priority was keeping things transparent, and explaining exactly what I did rather than "cleaning up" the data to much and possibly hurting its trustworthiness.

This is why my initial results had the outputs in their original three folders representing the three machines I distributed on. This is also why those results kept a typo in my initial root machine encoding, in the C++ source code as well as on the output files (it should have been `28329094430233769` rather than `28337890523255977`, and this affected all subsequent machines that kept this transition unchanged).

Both of those issues were later fixed, and the current output zip has all the results coming from an error-free run single machine.

### Keeping it Simple

While I needed to use bit-slicing techniques to improve my runtime, I tried to keep my code as clear and concise as possible. Without comments or empty lines, the C++ code only contains about 200 lines, and together with the comments its behavior should hopefully be pretty clear.

The attached python programs, both for distributing the enumeration work and for comparing the results with those of the original repository, are quite straight-forward and self-explanatory.

### Keeping it Fair

I tried to not "peek" at the original repository's code, and only verify and compare my outputs to its outputs. This turned out mostly well, using comparison to the 4-state case or to smaller sub-trees of the huge 5-state machine tree, and checking that the same machines are encountered.

One area that required me to actually compare source codes was the pruning of unneeded machines - it is not very clear what pruning was or was not applied on the original repository, so I had to check the source code in order to program my pruning logic to have similar functionality.

## What's Next

The way I see it, the following things to do are, roughly at that order:

1. Re-run with different pruning logic to match what was done in [Shawn Ligocki's results](https://github.com/sligocki/busy-beaver-data), which should allow us to resolve [this issue](https://github.com/bbchallenge/bbchallenge-seed/issues/2).
2. Re-run everything on a single thread, and update the output and verification code accordingly.
3. Add a detailed explanation of the pruning logic, in order to help others understand exactly which pruning logic is used and to replicate future results more easily.

Of course, any and all input and suggestions are very welcome!
