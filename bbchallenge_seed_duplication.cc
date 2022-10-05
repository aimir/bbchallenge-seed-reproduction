#include <tuple>
#include <iostream>
#include <fstream>
#include <list>
#include <queue>
#include <array>
#include <cstdlib>
#include <ctime>
#include <cassert>

// Time and space limits for 4-state machines:
int MAX_SPACE4 = 16;
int MAX_TIME4 = 107;

// Conjectured time and space limits for 5-state machines:
const int MAX_SPACE = 12289;
int MAX_TIME = 47176870;

// The tape of bits is encoded using 64-bit integers to save on memory access.
// We allocate twice as much as MAX_SPACE / 64 to make sure we have enough space
// both to the left and to the right of the initial position.
uint64_t tape[(MAX_SPACE * 2 + 1) / 64 + 1];

// State count - this code also works well if you change this value to 4:
#define STATES 5

// Each transition is encoded as a 5-bit integer as follows: the first 3 bits
// represent the next state, encoded numerically: A is 0, B is 1, end so on, up
// to E = 4, with the "halting state" encoded as H = 5. The next bit represents
// the bit to be written onto the tape (0 or 1) and the last one represents the
// direction to which we should move, with right encoded as 0 and left as 1.

// Machines are encoded as an integer, with each 5 bits representing an entry in
// the table of transitions. The first 5 bits represent the transition from
// state A when seeing a 0 on the tape, the next 5 bits represent the transition
// from A when seeing a 1, and then on to the transitions from B, and so on.

// Finally, an additional 8 bits per machine are used for two helper variables.
// The first 4 bits are used to represent the number of transitions that go to
// the halting state, in order to keep track of it and quickly prune machines
// with no transition to a halting state.

// The other 4 bits are used to denote the first fully halting state (or STATES
// if no such state exists). That is, the first state such that, whether we have
// a 0 or a 1 on the tape, we move from it to the halting state. We use this
// value to avoid checking redundant machines, since if multiple such fully
// halting states exist, they are symmetrical.

// Tape movement direction encoding:
#define R 0
#define L 1

// States' numeric values:
#define A 0
#define B 1
#define C 2
#define D 3
#define E 4
#define H 5

// The redurn value of our simulation function is encoded as a 8-bit integer.
// The first two bits are dedicated to the result of the simulation: 0 if the
// machine halts, 1 if we can be sure it didn't halt (which may happen if it
// used more than the 4-state time or space limits while only having 4 states),
// and 2 if we exceeded the conjectured time or space limits for a 5-state
// machine and are therefore undecided. The next 4 bits are used to denote the
// transition that led us to the halting state, if we halted: 1 bit for the
// value we read from the tape and 3 bits for the state. This will be useful in
// case we want to then modify this state so that it no longer halts.

// Machine simulation status:
#define HALTING 0
#define NONHALTING 1
#define UNDECIDED_SPACE 2
#define UNDECIDED_TIME 3

uint8_t simulate(uint64_t transition_int) {
    // Start in the middle of the tape, so that we have enough space on both the
    // left and the right. We encode the index using i1, i2 = divmod(index, 64).
    int index = MAX_SPACE, i1 = index / 64, i2 = index % 64;
    // The tape begins uninitialized, so we need to continously "clean" tape
    // cells to 0 whenever we first reach them
    tape[i1] = 0;
    uint64_t min_index_reached = index, max_index_reached = index, read_value = tape[i1], transition;
    signed char move_delta;
    uint8_t state = 0, new_state, transition_index, read_bit;
    // No point in simulating beyond the MAX_TIME for 5 states:
    for (int step = 0; step < MAX_TIME; ++step) {
        read_bit = (read_value >> i2) & 0b1;
        // The bit shift required to get the current transition is
        // 5 * (2 * current_state + current_read_bit)
        transition_index = 10 * state + 5 * read_bit;
        transition = (transition_int >> transition_index);
        new_state = transition & 0b111;
        if (new_state == H) {
            return (state << 3) + (((read_value >> i2) & 0b1) << 2) + (HALTING);
        }
        // We didn't halt, so move to this new state:
        state = new_state;
        // We modify by xor instead of by assignment, since we only want to
        // change the specific bit
        read_value ^= (read_bit ^ ((transition >> 3) & 0b1)) << i2;
        // We move by +1 if we had R = 0, and -1 if we had L = 1, so just take
        // 1 - 2 * transition_direction_encoded
        move_delta = 2 * ((transition >> 4) & 0b1);
        move_delta = 1 - move_delta;
        index += move_delta;
        i2 += move_delta;
        // Did we discover a new cell? If so, initialize it if needed:
        if (index < min_index_reached) {
            min_index_reached = index;
            if (i2 == -1) {
                tape[i1 - 1] = 0;
            }
        }
        if (index > max_index_reached) {
            max_index_reached = index;
            if (i2 == 64) {
                tape[i1 + 1] = 0;
            }
        }
        // Update i1 and i2 if i2 became smaller than 0 or larger than 63:
        if (i2 == 64) {
            tape[i1] = read_value;
            i1++;
            i2 = 0;
            read_value = tape[i1];
        }
        if (i2 == -1) {
            tape[i1] = read_value;
            i1--;
            i2 = 63;
            read_value = tape[i1];
        }
        // Did we go above the space or time limits for 4 transitions, while
        // only ever seeing 4 transitions?
        if ((transition_int >> 54) <= 4 && (max_index_reached - min_index_reached + 1 > MAX_SPACE4 || step + 1 > MAX_TIME4)) {
            return (NONHALTING);
        }
        // Did we go above the space limit?
        if (max_index_reached - min_index_reached + 1 > MAX_SPACE) {
            return (UNDECIDED_SPACE);
        }
    }
    // If we reached this point, we must have gone above the time limit:
    return (UNDECIDED_TIME);
}

// Prune machines with two identical (non-halting) states.
// We ignore states that may move to a halting state, because those might still
// be modified later in the enumeration tree.
bool prune_equivalent_states(uint64_t transition_int, uint8_t state) {
    // We take as input the integer encoding the current machine, as well as the
    // state we just changed - no point in checking other states, as they did
    // not change from the last check.
    uint64_t transition = transition_int >> (10 * state);
    bool x00 = (transition >> 3) & 0b1;
    bool x01 = (transition >> 8) & 0b1;
    bool y00 = (transition >> 4) & 0b1;
    bool y01 = (transition >> 9) & 0b1;
    uint8_t z00 = transition & 0b111;
    uint8_t z01 = (transition >> 5) & 0b111;
    // Is this newly-changed state halting, given a certian tape bit?
    if (z00 == H || z01 == H) {
        return false;
    }
    bool x10, x11, y10, y11;
    uint8_t z10, z11, z00_canonicalized, z01_canonicalized, z10_canonicalized, z11_canonicalized, maximal, minimal;
    for(uint8_t other_state = 0; other_state < STATES; other_state++) {
        if (state == other_state) {
            continue;
        }
        uint64_t new_transition = transition_int >> (10 * other_state);
        bool x10 = (new_transition >> 3) & 0b1;
        bool x11 = (new_transition >> 8) & 0b1;
        bool y10 = (new_transition >> 4) & 0b1;
        bool y11 = (new_transition >> 9) & 0b1;
        uint8_t z10 = new_transition & 0b111;
        uint8_t z11 = (new_transition >> 5) & 0b111;
        // Is the other state halting?
        if (z10 == H || z11 == H) {
            continue;
        }
        // We want to just check if they are equal, but we want the next states
        // z to be considered equal even if, say, z00 == new_state, while
        // z10 == state. Therefore, we canonicalize the z values before
        // actually comparing them:
        minimal = std::min(state, other_state);
        maximal = std::max(state, other_state);
        z00_canonicalized = (z00 == maximal) ? minimal : z00;
        z01_canonicalized = (z01 == maximal) ? minimal : z01;
        z10_canonicalized = (z10 == maximal) ? minimal : z10;
        z11_canonicalized = (z11 == maximal) ? minimal : z11;
        if (
            x00 == x10 &&
            x01 == x11 &&
            y00 == y10 &&
            y01 == y11 &&
            z00_canonicalized == z10_canonicalized &&
            z01_canonicalized == z11_canonicalized
        ) {
            // state and new_state are equivalent.
            return true;
        }
    }
    // No new_state was found to be equivalent to the newly-changed state.
    return false;
}

// prune states with no effect - that is, non-halting states that don't change
// the tape and send us right back to the direction we came from.
// We again ignore states that might move to a halting state, for the same
// reason as in the pruning function above.
bool prune_useless_states(uint64_t transition_int, uint8_t state, bool read_bit) {
    // We receive as input the encoded machine, as well as the index of the
    // newly-changed transition (indexed by state and currently-read tape bit).
    uint64_t transition = transition_int >> (10 * state + 5 * read_bit);
    bool tape_direction = (transition >> 4) & 0b1;
    uint8_t new_state = transition & 0b111;
    // We check if new_state has no effect:
    uint64_t new_transition = transition_int >> (10 * new_state);
    // For new_state to have no effect, the following all need to happen:
    // If we read 0 from the tape we write 0 to the tape right back again:
    bool x00 = (new_transition >> 3) & 0b1;
    // Similarly, if we read 1 we write 1 back:
    bool x01 = (new_transition >> 8) & 0b1;
    // We go back in the opposite direction to tape_direction, wheather we read
    // 0 or 1 from the input tape:
    bool y00 = (new_transition >> 4) & 0b1;
    bool y01 = (new_transition >> 9) & 0b1;
    // Wheather we read 0 or 1, we move to the same (non-halting) state.
    uint8_t z00 = new_transition & 0b111;
    uint8_t z01 = (new_transition >> 5) & 0b111;
    // If all of those apply, this new transition into new_state is equivalent
    // to just going directly into the "newer-state" we get to after new_state:
    return !(z00 == H || z01 == H || x00 != 0 || x01 != 1 || y00 == tape_direction || y01 == tape_direction || z00 != z01);
}

int main() {
    // Initially, all transitions are to the halting state.
    // However, due to symmetry, we can decide the first transition to be from
    // A to B, writing a 1 and moving to the right, without loss of generality,
    // so we might as well start with this state initialized.
    uint64_t root = (
        (((R << 4) + (1 << 3) + (uint64_t)B) <<  0) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) <<  5) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 10) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 15) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 20) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 25) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 30) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 35) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 40) +
        (((0 << 4) + (0 << 3) + (uint64_t)H) << 45) +
        (9ull << 50) +
        (1ull << 54)
    );
    std::ofstream myfile;
    myfile.open(std::to_string(root) + ".txt");
    std::queue<uint64_t> machine_queue;
    uint64_t first_undefined_state, child;
    machine_queue.push(root);
    std::srand(std::time(nullptr));
    while(!machine_queue.empty()) {
        // Simulate the first machine in the queue:
        uint64_t machine = machine_queue.front();
        uint8_t status = simulate(machine);
        myfile << machine << " " << (int)status << std::endl;
        if ((status & 0b11) == HALTING) {
            // Should we generate its children?
            uint8_t state = (status >> 3) & 0b111;
            bool read_bit = (status >> 2) & 0b1;
            uint8_t transition_index = state * 2 + read_bit;
            if (((machine >> 50) & 0b1111) > 1) {
                // If there are several undefined states, only consider the
                // first one - they are all equivalent
                first_undefined_state = machine >> 54;
                first_undefined_state += (first_undefined_state == state ? 1 : 0);
                for (uint64_t new_write_bit = 0; new_write_bit < 2; new_write_bit++) {
                    for (uint64_t new_direction = 0; new_direction < 2; new_direction++) {
                        for (uint64_t new_state = 0; new_state < STATES; new_state++) {
                            // Initialize the child to be identical to
                            // its parent:
                            child = machine;
                            // Add the new transition:
                            child &= 0b11111111111111111111111111111111111111111111111111ull ^ (0b11111ull << (5 * transition_index));
                            child |= (new_direction << (4 + 5 * transition_index)) + (new_write_bit << (5 * transition_index + 3)) + (new_state << (5 * transition_index));
                            // Update the new number of undefined
                            // transitions to be one less than at
                            // the parent:
                            child |= (((machine >> 50) & 0b1111) - 1) << 50;
                            // Update the first undefined state:
                            child |= (first_undefined_state) << 54;
                            // Should it be pruned?
                            if ((new_state <= first_undefined_state) && (!prune_equivalent_states(child, state)) && (!prune_useless_states(child, state, read_bit))) {
                                // If not, add it to the queue:
                                machine_queue.push(child);
                            }
                        }
                    }
                }
            }
        }
        machine_queue.pop();
    }
    myfile << "done" << std::endl;
    myfile.close();
    return 0;
}
