#include <atomic>
#include <mutex>
#include <optional>
#include <utility>

template<typename T>
class LazyList {
    struct Node {
        const T data;
        const unsigned key;

        std::atomic<Node*> next{nullptr};

        std::mutex mtx;
        std::atomic<bool> mark{false};

        Node() : key(0) {}
        Node(T data_, unsigned key_) : data(std::move(data_)), key(key_) {}

        void lock() {
            mtx.lock();
        }

        void unlock() {
            mtx.unlock();
        }

    };

    std::atomic<Node*> head;

public:

    LazyList() {
        Node* dummy = new Node();
        head.store(dummy); 
    }

    LazyList(const LazyList&) = delete;
    LazyList& operator=(const LazyList&) = delete;

    LazyList(LazyList&&) = delete;
    LazyList& operator=(LazyList&&) = delete;

    bool add(T item, unsigned key) {
        while (true) {
            Node* pred = head.load();
            Node* curr = pred->next.load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next.load();
            }
            {
                std::lock_guard<Node> lock_pred(*pred);
                {
                    if (curr) {
                        std::lock_guard<Node> lock_curr(*curr);
                        if (validate(pred, curr)) {
                            if (curr->key == key) {
                                return false;
                            } else {
                                Node* new_node = new Node(item, key);
                                new_node->next.store(curr);
                                pred->next.store(new_node);
                                return true;
                            }
                        }
                    } else if (validate(pred)){
                        Node* new_node = new Node(item, key);
                        pred->next.store(new_node);
                        return true;
                    }
                }
            }
        }
    }

    std::optional<T> remove(unsigned key) {
        while (true) {
            Node* pred = head.load();
            Node* curr = pred->next.load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next.load();
            }
            if (!curr) {
                return std::nullopt;
            }
            {
                std::lock_guard<Node> lock_pred(*pred);
                {
                    std::lock_guard<Node> lock_curr(*curr);
                    if (validate(pred, curr)) {
                        if (curr->key != key) {
                            return std::nullopt;
                        } else {
                            curr->mark.store(true);
                            pred->next.store(curr->next.load());
                            return curr->data;
                        }
                    }
                }
            }
        }
    }

    bool contains(unsigned key) {
        Node* curr = head.load();
        while (curr && curr->key < key) {
            curr = curr->next.load();
        }
        return curr && curr->key == key && !curr->mark.load();
    }

private:

    bool validate(Node* last) {
        return !last->mark.load() && last->next.load() == nullptr;
    }

    bool validate(Node* pred, Node* curr) { 
        return !pred->mark.load() && !curr->mark.load() && pred->next.load() == curr;
    }
};