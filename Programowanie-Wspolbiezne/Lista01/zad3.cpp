#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <random>
#include <memory>

int const NUMBER_OF_BANK_ACCOUNTS = 700;

int const NUMBER_OF_ACCOUNTANTS = 20; 
int const NUMBER_OF_READERS = 100;

int const DURATION = 10; // s

int const MIN_SLEEP_TIME = 1; // 1 ms
int const MAX_SLEEP_TIME = 5; // 5 ms

int const AUDITOR_TIME = 10; // 10 ms
int const MAX_TRIES = 5;

int const STARTING_MONEY = 10'000'00; // 10'000 PLN
int const MIN_TRANSACTION = 100; // 1 PLN
int const MAX_TRANSACTION = 5'000'00; // 5'000 PLN


std::chrono::steady_clock::time_point closing_time;

class BankAccount {
    int id;
    int balance;

    mutable std::shared_timed_mutex mtx;

    void deposit(int amount) {
        balance += amount;
    }

    bool withdraw(int amount) {
        if (balance < amount) return false;
        balance -= amount;
        return true;
    }

public:    
    BankAccount(int _id, int _balance) : id(_id), balance(_balance) {}

    int get_id() const {
        return id;
    }

    int get_balance() const {
        std::shared_lock<std::shared_timed_mutex> lock(mtx);
        return balance;
    }

    bool try_get_balance(int timeout_ms, int& out_balance) const {
        std::shared_lock<std::shared_timed_mutex> lock(mtx, std::defer_lock);
        if (lock.try_lock_for(std::chrono::milliseconds(timeout_ms))) {
            out_balance = balance;
            return true;
        }
        return false;
    }

    friend bool transfer(BankAccount& from, BankAccount& to, int amount, int sleep_ms);
};

std::vector<std::unique_ptr<BankAccount>> accounts;

bool transfer(BankAccount& from, BankAccount& to, int amount, int sleep_ms) {

    std::unique_lock<std::shared_timed_mutex> lock_from(from.mtx, std::defer_lock);
    std::unique_lock<std::shared_timed_mutex> lock_to(to.mtx, std::defer_lock);

    if (from.get_id() == to.get_id()) {
        return false;
    }
    else if (from.get_id() < to.get_id()) {
        lock_from.lock();
        lock_to.lock();
    } else {
        lock_to.lock();
        lock_from.lock();
    }

    if (!from.withdraw(amount)) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

    to.deposit(amount);

    return true;
}

void accountant() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> acc_dist(0, NUMBER_OF_BANK_ACCOUNTS - 1);
    std::uniform_int_distribution<int> amount_dist(MIN_TRANSACTION, MAX_TRANSACTION);
    std::uniform_int_distribution<int> sleep_dist(MIN_SLEEP_TIME, MAX_SLEEP_TIME);

    while (std::chrono::steady_clock::now() < closing_time) {
        int from = acc_dist(rng);
        int to = acc_dist(rng);
        int amount = amount_dist(rng);
        int sleep_ms = sleep_dist(rng);

        if (!transfer(*accounts[from], *accounts[to], amount, sleep_ms)) {
            std::this_thread::yield();
        } else {
            // std::cout << "Accountant " << std::this_thread::get_id() << ": From " << from << " To" << to << " amount = " << amount << "\n";
        }
    }
}

void reader() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> acc_dist(0, NUMBER_OF_BANK_ACCOUNTS - 1);
    while (std::chrono::steady_clock::now() < closing_time) {
        int id = acc_dist(rng);
        int balance = accounts[id]->get_balance();
        // std::cout << "Reader " << std::this_thread::get_id() << ": Account " << id << " Balance = "<< balance << "\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void audit(int& successful_cycles) {
    int cycle_numer = 1;
    while (std::chrono::steady_clock::now() < closing_time) {
        long long sum = 0;
        std::queue<std::pair<int, int>>  lookup;
        for (int i = 0; i < NUMBER_OF_BANK_ACCOUNTS; i++) {
            lookup.push({i, 0});
        }

        bool successful_cycle = true;

        while (!lookup.empty()) {
            auto [id, tries] = lookup.front();
            lookup.pop();

            int current_balance = 0;
            if (accounts[id]->try_get_balance(AUDITOR_TIME, current_balance)) {
                sum += current_balance;
            } else if (++tries >= MAX_TRIES) {
                std::cout << "Auditor: Cycle " << cycle_numer << " error! STARVATION!\n";
                successful_cycle = false;
                break;
            } else {
                lookup.push({id, tries});
            }
        }

        if (lookup.empty()) {
            std::cout << "Auditor: Cycle " << cycle_numer << " completed SUM = " << sum << "\n";
            successful_cycles++;
        }

        cycle_numer++;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {

    std::cout << "Make Market!\n";

    for (int i = 0; i < NUMBER_OF_BANK_ACCOUNTS; i++) {
        accounts.push_back(std::make_unique<BankAccount>(i, STARTING_MONEY)); 
    }

    std::vector<std::thread> accountants, readers;

    closing_time = std::chrono::steady_clock::now() + std::chrono::seconds(DURATION);

    for (int i = 0; i < NUMBER_OF_ACCOUNTANTS; i++) {
        accountants.push_back(std::thread(accountant));
    }

    for (int i = 0; i < NUMBER_OF_READERS; i++) {
        readers.push_back(std::thread(reader));
    }

    int successful_cycles = 0;
    std::thread auditor(audit, std::ref(successful_cycles));

    for (int i = 0; i < NUMBER_OF_ACCOUNTANTS; i++) {
        accountants[i].join();
    }

    for (int i = 0; i < NUMBER_OF_READERS; i++) {
        readers[i].join();
    }

    auditor.join();

    long long sum = 0;
    long long expected_sum = NUMBER_OF_BANK_ACCOUNTS * STARTING_MONEY;

    for (int i = 0; i < NUMBER_OF_BANK_ACCOUNTS; i++) {
        sum += accounts[i]->get_balance();
    }

    std::cout << "Close Market!\n";
    
    std::cout << ((sum == expected_sum) ? "Everything fine\n" : "Some money lost!!!\n");

    return 0;
}