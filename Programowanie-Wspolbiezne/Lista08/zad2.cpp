#include <atomic>
#include <chrono>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <mutex>

class TransactionConsensus {
private:
    std::string payload; // Ordinary, non-atomic data
    double amount;      // Ordinary, non-atomic data
    // State: 0 means no proposal; >0 is the number of supporting votes
    std::atomic<uint32_t> votes{0};
public:
    // The initiating thread publishes a transaction proposal
    void propose(const std::string& p, double a) {
        payload = p;
        amount = a;
        // 1. EXERCISE: Set 'votes' to 1. Use the weakest
        // suitable memory order that publishes 'payload' and 'amount'.
        // YOUR CODE HERE

        votes.store(1, std::memory_order_release);
        // ensure the changes occur before publishing 'votes'
        // so reading the new vote count also observes the initialized data
    }
    // Worker threads add their support (called concurrently)
    void vote_up() {
        // 2. EXERCISE: Increment 'votes' by 1.
        // Use a suitable atomic operation, such as RMW (read-modify-write).
        // YOUR CODE HERE

        votes.fetch_add(1, std::memory_order_relaxed);
        // relaxed is sufficient here because the operation extends a release sequence
        // these threads do not access the data; they pass synchronization along

    }

    void vote_up_check() {

        uint32_t expected = votes.load(std::memory_order_acquire);
        // we need synchronization to inspect payload and amount

        if (expected > 0) {
            // a validation rule could access the variables here
            // "One designated thread creates a single transaction in this concurrent system"
            // so these reads do not introduce a data race
            if (amount > 42.0) {
                // use CAS instead of fetch_add; we cannot split this into two relaxed operations
                // because that would introduce a race condition
                while (!votes.compare_exchange_weak(expected, expected + 1,
                                                        std::memory_order_relaxed,
                                                        std::memory_order_relaxed));
                // if CAS fails because another thread incremented the counter first,
                // retry the loop with the updated 'expected'

            }
        }

    }

    // The executor thread tries to execute the transaction
    bool try_execute(std::string& out_p, double& out_a, uint32_t& out_level) const {
        // 3. EXERCISE: Read 'votes'. Use a suitable memory order
        // that synchronizes with publication.
        // YOUR CODE HERE
        uint32_t level = votes.load(std::memory_order_acquire);
        // last element release-sequence
        if (level > 0) {
            out_p = payload;
            out_a = amount;
            out_level = level;
            return true;
        }
        return false;
    }
};

// Program start time (used to measure elapsed time)
inline const auto start_time = std::chrono::high_resolution_clock::now();

class SafePrint {
public:
    SafePrint() {
        auto now = std::chrono::high_resolution_clock::now();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();

        buffer << "[" << std::setw(8) << micros << " us] ";
    }

    ~SafePrint() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << buffer.str() << std::flush;
    }

    template<typename T>
    SafePrint& operator<<(const T& msg) {
        buffer << msg;
        return *this;
    }

private:
    static std::mutex mtx;
    std::ostringstream buffer;
};

std::mutex SafePrint::mtx;

TransactionConsensus consensus;
int const NUM_VOTERS = 5;

void initiator_task() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SafePrint() << "[Initiator] New proposal...\n";
    consensus.propose("Purchase XYZ", 100.0);
}

void voter_task(int id) {
    std::string p;
    double a;
    uint32_t l;

    while (!consensus.try_execute(p, a, l)) {
        std::this_thread::yield();
    }

    SafePrint() << "[Voter " << id << "] Reviewed the proposal '" << p  << "'. Supports it!\n";

    consensus.vote_up();
}

void executor_task() {
    std::string p;
    double a;
    uint32_t l;

    uint32_t const REQUIRED_VOTES = 1 + NUM_VOTERS;
    // everyone must agree

    while (true) {
        if (consensus.try_execute(p, a, l)) {
            if (l >= REQUIRED_VOTES) {
                SafePrint() << "[EXECUTOR] CONSENSUS REACHED!\n";
                SafePrint() << "-> Transaction: " << p << "\n";
                SafePrint() << "-> Amount:      " << a << " PLN\n";
                SafePrint() << "-> Votes:       " << l << "/" << REQUIRED_VOTES << "\n";
                break;
            }
        }
        std::this_thread::yield();
    }
}

int main() {
    std::thread executor(executor_task);

    std::vector<std::thread> voters;
    for (int i = 1; i <= NUM_VOTERS; ++i) {
        voters.emplace_back(voter_task, i);
    }

    std::thread initiator(initiator_task);

    initiator.join();
    for (auto& v : voters) {
        v.join();
    }
    executor.join();

    std::cout << "\nTest completed successfully.\n";
    return 0;
}
