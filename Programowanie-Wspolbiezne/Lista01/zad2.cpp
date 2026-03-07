#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <memory>

int const NUMBER_OF_BANK_ACCOUNTS = 700;
int const NUMBER_OF_ACCOUNTANTS = 20; 
int const DURATION = 5;

int const MIN_SLEEP_TIME = 1; // 1 ms
int const MAX_SLEEP_TIME = 5; // 5 ms

int const STARTING_MONEY = 10'000'00; // 10'000 PLN
int const MIN_TRANSACTION = 100; // 1 PLN
int const MAX_TRANSACTION = 5'000'00; // 5'000 PLN

std::chrono::steady_clock::time_point closing_time;

class BankAccount {
    int id;
    int balance;
    
    mutable std::mutex mtx;

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
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }

    friend bool transfer(BankAccount& from, BankAccount& to, int amount, int sleep_ms);
};

std::vector<std::unique_ptr<BankAccount>> accounts;

bool transfer(BankAccount& from, BankAccount& to, int amount, int sleep_ms) {

    std::unique_lock<std::mutex> lock_from(from.mtx, std::defer_lock);
    std::unique_lock<std::mutex> lock_to(to.mtx, std::defer_lock);

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

void maker() {
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
        }
    }
}

int main() {

    std::cout << "Make Market!\n";

    for (int i = 0; i < NUMBER_OF_BANK_ACCOUNTS; i++) {
        accounts.push_back(std::make_unique<BankAccount>(i, STARTING_MONEY)); 
    }

    std::vector<std::thread> accountants;

    closing_time = std::chrono::steady_clock::now() + std::chrono::seconds(DURATION);

    for (int i = 0; i < NUMBER_OF_ACCOUNTANTS; i++) {
        accountants.push_back(std::thread(maker));
    }

    for (int i = 0; i < NUMBER_OF_ACCOUNTANTS; i++) {
        accountants[i].join();
    }

    long long sum = 0;
    long long expected_sum = NUMBER_OF_BANK_ACCOUNTS * STARTING_MONEY;

    for (int i = 0; i < NUMBER_OF_BANK_ACCOUNTS; i++) {
        sum += accounts[i]->get_balance();
    }

    std::cout << "Close Market!\n";
    
    std::cout << ((sum == expected_sum) ? "Everything fine\n" : "Some money lost!!!\n");

    return 0;
}