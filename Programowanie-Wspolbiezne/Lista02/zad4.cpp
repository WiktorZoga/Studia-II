#include <iostream>
#include <vector>
#include <thread>
#include <barrier>
#include <random>
#include <chrono>

class GameOfLife {
public:
    struct CompletionStep {
        GameOfLife* game_instance;
        void operator()() noexcept {
            game_instance->end_of_phase_action();
        }
    };
private:
    int const width, height;
    int num_threads;
    int num_iterations;

    std::vector<uint8_t> grid_a;
    std::vector<uint8_t> grid_b;

    std::vector<uint8_t>* read_grid;
    std::vector<uint8_t>* write_grid;

    std::barrier<CompletionStep> barrier;
    std::vector<std::jthread> workers;

    inline int get_index(int row, int col) const {
        if (row == -1) row = height - 1;
        else if (row == height + 1) row = 1;
        if (col == -1) col = width - 1;
        else if (col == width + 1) col = 1;
        return row * width + col;
    }

    void end_of_phase_action() noexcept {
        std::swap(read_grid, write_grid);
        // print_grid();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void worker_thread(int thread_id, int start_row, int end_row) {
        for (int iter = 0; iter < num_iterations; iter++) {
            for (int row = start_row; row < end_row; row++) {
                for (int col = 0; col < width; col++) {
                    int index = get_index(row, col);
                    int count = count_alive(row, col);
                    (*write_grid)[index] = 
                        (count == 3 || (count == 2 && ((*read_grid)[index] == 1))) ? 1 : 0;
                }
            }
            barrier.arrive_and_wait();
        }
    }

    int count_alive(int row, int col) const {
        int count = 0;
        for (auto dh : {-1, 0, 1}) {
            for (auto dw: {-1, 0, 1}) {
                count += (*read_grid)[get_index(row + dh, col + dw)];
            }
        }
        count -= (*read_grid)[get_index(row, col)];
        return count;
    }

public:
    GameOfLife(int w, int h, int requested_threads, int iterations) : 
        width(w), height(h), num_threads(std::max(1, std::min({(int)std::thread::hardware_concurrency(), requested_threads, h}))), num_iterations(iterations),
        grid_a(w * h, 0),                        
        grid_b(w * h, 0),                        
        read_grid(&grid_a),                      
        write_grid(&grid_b),                     
        barrier(num_threads, CompletionStep{this})   
    {
        workers.reserve(num_threads);
    }

    void run() {

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 1);
        
        for (int i = 0; i < width * height; i++) {
            grid_a[i] = dist(rng);
        }

        int base_chunk = height / num_threads;
        int reminder = height % num_threads;

        int current_row = 0;
        for (int i = 0; i < num_threads; i++) {
            int start_row = current_row;
            int extra_row = (i < reminder) ? 1 : 0;
            int end_row = start_row + base_chunk + extra_row;

            workers.emplace_back(&GameOfLife::worker_thread, this, i, start_row, end_row);
            current_row = end_row;
        }

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void print_grid() const {
        int index = 0;
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                if ((*read_grid)[index] == 1) {
                    std::cout << "X";
                } else {
                    std::cout << "O";
                }
                index++;
            }
            std::cout << "\n";
        }
        for (int i = 0; i < width; i++) {
            std::cout << "-";
        }
        std::cout << "\n";
    }
};

int main() {
    int width = 1000;
    int height = 1000;
    int threads = 10;
    int iterations = 1000;

    GameOfLife game(width, height, threads, iterations);
    game.run();

    return 0;
}