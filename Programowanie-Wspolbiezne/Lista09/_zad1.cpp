    #include <atomic>
    #include <memory>
    #include <optional>

    template<typename T>
    class Queue {
        struct Node {
            std::shared_ptr<T> data;
            std::atomic<std::shared_ptr<Node>> next; 

            Node() : data(nullptr), next(nullptr) {}
            Node(T val) : data(std::make_shared<T>(std::move(val))), next(nullptr) {}
        };

        std::atomic<std::shared_ptr<Node>> head;
        std::atomic<std::shared_ptr<Node>> tail;

    public:

        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;
        Queue(Queue&&) = delete;
        Queue& operator=(Queue&&) = delete;

        Queue() {
            auto dummy = std::make_shared<Node>();
            head.store(dummy, std::memory_order_relaxed);
            tail.store(dummy, std::memory_order_relaxed);
        }

        void enqueue(T val) {
            auto new_node = std::make_shared<Node>(val);
            while (true) {
                auto current_tail = tail.load(std::memory_order_acquire); 
                auto current_tail_next = current_tail->next.load(std::memory_order_acquire); // relaxed byłoby ale to shared_ptr

                if (current_tail == tail.load(std::memory_order_relaxed)) {
                    if (current_tail_next == nullptr) {
                        std::shared_ptr<Node> expected = nullptr;
                        if (current_tail->next.compare_exchange_weak(expected, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                            tail.compare_exchange_weak(current_tail, new_node, std::memory_order_release, std::memory_order_relaxed); 
                            return;
                        }
                    } else {
                        tail.compare_exchange_weak(current_tail, current_tail_next, std::memory_order_release, std::memory_order_relaxed);
                    }
                }
            }
        }

        std::optional<T> dequeue() {
            while (true) {
                auto current_head = head.load(std::memory_order_acquire); 
                auto current_tail = tail.load(std::memory_order_acquire); // relaxed byłoby ale to shared_ptr
                auto current_head_next = current_head->next.load(std::memory_order_acquire);

                if (current_head == head.load(std::memory_order_relaxed)) { 
                    if (current_head == current_tail) {
                        if (current_head_next == nullptr) {
                            return std::nullopt;
                        } 
                        tail.compare_exchange_weak(current_tail, current_head_next, std::memory_order_release, std::memory_order_relaxed); 
                    } else {
                        T result = *(current_head_next->data);
                        if (head.compare_exchange_weak(current_head, current_head_next, std::memory_order_release, std::memory_order_relaxed)) {
                            return result;
                        }
                    }
                }
            }
        }
    };