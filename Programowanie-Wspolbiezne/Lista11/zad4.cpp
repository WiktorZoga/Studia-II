#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <random>
#include <cassert>

template<typename T>
class lock_free_stack
{
private:
    struct node;
    
    struct counted_node_ptr
    {
        int external_count;
        node* ptr;
    };

    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        
        node(T const& data_):
            data(std::make_shared<T>(data_)),
            internal_count(0)
        {}
    };

    std::atomic<counted_node_ptr> head;

    void increase_head_count(counted_node_ptr& old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        }
        while(!head.compare_exchange_strong(old_counter, new_counter));
        old_counter.external_count = new_counter.external_count;
    }

public:
    lock_free_stack() {
        counted_node_ptr init_head;
        init_head.ptr = nullptr;
        init_head.external_count = 0;
        head.store(init_head);
    }

    ~lock_free_stack()
    {
        while(pop());
    }

    void push(T const& data)
    {
        counted_node_ptr new_node;
        new_node.ptr = new node(data);
        new_node.external_count = 1;
        new_node.ptr->next = head.load();
        while(!head.compare_exchange_weak(new_node.ptr->next, new_node));
    }

    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head = head.load();
        for(;;)
        {
            increase_head_count(old_head);
            node* const ptr = old_head.ptr;
            if(!ptr)
            {
                return std::shared_ptr<T>();
            }
            if(head.compare_exchange_strong(old_head, ptr->next))
            {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                
                int const count_increase = old_head.external_count - 2;
                
                // MIEJSCE DO SABOTAŻU #1
                // if(ptr->internal_count.fetch_add(count_increase) == -count_increase) // originalne
                if(ptr->internal_count.fetch_add(count_increase) == -count_increase) // tutaj jakieś zmiany (np == -count_increase+1)
                {
                    delete ptr;
                }
                return res;
            }
            // MIEJSCE DO SABOTAŻU #2 
            // else if(ptr->internal_count.fetch_sub(1) == 1)  // originalne
            else if(ptr->internal_count.fetch_sub(1) == 1) // tutaj jakieś zmiany (np == 2)
            {
                delete ptr;
            }
        }
    }
};

int main() {
    std::cout << "[TEST] Starting lock-free stack test with reference counting...\n";
    lock_free_stack<int> stack;
    
    std::atomic<bool> start_signal{false};
    std::atomic<bool> stop_flag{false};

    auto worker = [&](int id) {
        while(!start_signal.load()) std::this_thread::yield();
        
        std::mt19937 rng(id);
        std::uniform_int_distribution<int> dist(1, 100);

        for (int i = 0; i < 5000; ++i) {
            if (dist(rng) % 2 == 0) {
                stack.push(i);
            } else {
                stack.pop();
            }
        }
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker, i);
    }

    start_signal.store(true);
    threads.clear(); 

    std::cout << "[TEST] Finished successfully. No crashes.\n";
    return 0;
}