#include <array>
#include <atomic>
#include <optional>
#include <thread>
#include <mutex>

template<typename T>
class Queue {
private:
    std::vector<T> buff;
    std::atomic<size_t> head{0}, tail{0};
    size_t const size;
public:

    Queue(size_t size_) : size(size_ + 1) {
        buff.resize(size);
    }

    bool push(const T& val) {
        size_t current_head = head.load();
        size_t current_tail = tail.load();
        size_t new_tail = next(current_tail);

        if (new_tail == current_head) {
            return false;
        }

        buff[current_tail] = val;
        tail.store(new_tail);

        return true;
    }

    std::optional<T> pop() {
        size_t current_head = head.load();
        size_t current_tail = tail.load();

        if (current_head == current_tail) {
            return std::nullopt;
        }

        std::optional<T> opt{buff[current_head]};
        size_t new_head = next(current_head);
        head.store(new_head);
        return opt;
    }

private:
    size_t next(size_t it) const {
        return (it + 1) % size;
    }
}