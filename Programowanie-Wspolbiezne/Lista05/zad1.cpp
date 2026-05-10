#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <vector>
#include <memory>
#include <utility>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>

class Vertex {
private:
    std::unordered_set<int> neighbours;
    mutable std::shared_mutex mutex;

public:
    int id;
    
    explicit Vertex(int id_) : id(id_) {} 

    Vertex(const Vertex&) = delete;
    Vertex& operator=(const Vertex&) = delete;

    Vertex(Vertex&&) = delete;
    Vertex& operator=(Vertex&&) = delete;

friend class ConcurrentGraph;
    
};

class ConcurrentGraph {
private:
    std::vector<std::unique_ptr<Vertex>> vertices;
public:
    int const size;

    explicit ConcurrentGraph(int size_) : size(size_) {
        for (int i = 0; i < size; i++) {
            vertices.push_back(std::make_unique<Vertex>(i));
        }
    }

    void add_edge(int id_a, int id_b) {
        if (id_a == id_b) return;
        if (id_a > id_b) std::swap(id_a, id_b);
        std::unique_lock<std::shared_mutex> lock_a(vertices[id_a]->mutex);
        std::unique_lock<std::shared_mutex> lock_b(vertices[id_b]->mutex);
        vertices[id_a]->neighbours.insert(id_b);
        vertices[id_b]->neighbours.insert(id_a);
    }

    void remove_edge(int id_a, int id_b) {
        if (id_a == id_b) return;
        if (id_a > id_b) std::swap(id_a, id_b);
        std::unique_lock<std::shared_mutex> lock_a(vertices[id_a]->mutex);
        std::unique_lock<std::shared_mutex> lock_b(vertices[id_b]->mutex);
        vertices[id_a]->neighbours.erase(id_b);
        vertices[id_b]->neighbours.erase(id_a);
    }

    bool are_connected(int id_a, int id_b) const {
        if (id_a == id_b)   return true;
        std::shared_lock<std::shared_mutex> lock_a(vertices[id_a]->mutex);
        return vertices[id_a]->neighbours.contains(id_b);
    }

    bool is_path(const std::vector<int>& p) const {
        if (p.size() < 2) {
            return true;
        }
        std::vector<int> locking_order = p;
        std::sort(begin(locking_order), end(locking_order));
        locking_order.erase(std::unique(locking_order.begin(), locking_order.end()), locking_order.end());

        std::vector<std::shared_lock<std::shared_mutex>> locks;
        locks.reserve(locking_order.size());
        for (int id: locking_order) {
            locks.emplace_back(vertices[id]->mutex);
        }
        int m = p.size();
        for (int i = 0; i < m - 1; i++) {
            int u = p[i], v = p[i + 1];
            if (!vertices[u]->neighbours.contains(v)){
                return false;
            }
        }
        return true;
    }

    std::vector<int> get_common_neighbors(int id_a, int id_b) const {
        if (id_a == id_b) return {};
        if (id_a > id_b) std::swap(id_a, id_b);
        std::shared_lock<std::shared_mutex> lock_a(vertices[id_a]->mutex);
        std::shared_lock<std::shared_mutex> lock_b(vertices[id_b]->mutex);
        std::vector<int> common_neighbors;
        bool a_leq_b = (vertices[id_a]->neighbours.size() <= vertices[id_b]->neighbours.size());
        int id_0 = a_leq_b ? id_a : id_b;
        int id_1 = a_leq_b ? id_b : id_a;
        for (auto& id: vertices[id_0]->neighbours) {
            if (id_0 == id || id_1 == id) continue;
            if (vertices[id_1]->neighbours.contains(id)) common_neighbors.emplace_back(id);
        }
        return common_neighbors;
    }

    void print_node_stats(int id) const {
        std::shared_lock<std::shared_mutex> lock(vertices[id]->mutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    bool is_path_unsafe(int from, int to) const {
        std::vector<bool> visited(size, false);

        auto dfs = [&](auto& self, int u) {
            visited[u] = true;
            std::shared_lock<std::shared_mutex> lock(vertices[u]->mutex);
            if (u == to) {
                return true;
            }
            bool found = false;
            for (int v: vertices[u]->neighbours) {
                if (visited[v]) continue;
                found = self(self, v);
                if (found) break;
            }
            return found;
        };

        return dfs(dfs, from);
    }

    std::vector<int> get_neighbors(int id) const {
        std::shared_lock<std::shared_mutex> lock(vertices[id]->mutex);
        return std::vector<int>(vertices[id]->neighbours.begin(), vertices[id]->neighbours.end());
    }

    bool is_path_safe(int from, int to) const {
        std::vector<bool> visited(size, false);
        std::vector<int> path;
        path.reserve(size);

        auto dfs = [&](auto& self, int u) {
            if (u == to) return true;
            visited[u] = true;
            std::vector<int> neighbours = get_neighbors(u);
            for (int v: neighbours) {
                if (visited[v]) continue;
                if (self(self, v)) {
                    path.push_back(v);
                    return true;
                }
            }
            return false;
        };

        bool optimistic_path_can_be_found = true;

        while (optimistic_path_can_be_found) { 
            visited.assign(size, false);
            bool is_path_found = dfs(dfs, from);
            if (!is_path_found) { // even optimistic search didn't find any path
                return false;
            } // optimitic search found some path
            path.push_back(from);
            optimistic_path_can_be_found = !is_path(path);
            path.clear();
        }
        return true;
    }
};

// Globalny punkt startu programu (do mierzenia relatywnego czasu)
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


int const N = 100;

void choas_builder(std::stop_token stoken, ConcurrentGraph& g) {
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dist(0, g.size-1);

    while (!stoken.stop_requested()) {
        int a = dist(gen), b = dist(gen);
        g.add_edge(a, b);
        SafePrint() << "[BUILDER] Added edge: " << a << " -- " << b << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void chaos_destroyer(std::stop_token stoken, ConcurrentGraph& g) {
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dist(0, g.size-1);

    while (!stoken.stop_requested()) {
        int a = dist(gen), b = dist(gen);
        g.remove_edge(a, b);
        SafePrint() << "[DESTROYER] Removed edge: " << a << " -- " << b << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void path_finder(ConcurrentGraph& g) {
    while (!g.is_path_safe(0, g.size - 1)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    SafePrint() << "[FINDER] Found path between 0 and " << g.size - 1 << "!!!\n";
}

void sneaky_builder(std::stop_token stoken, ConcurrentGraph& g) {
    int n = g.size;
    for (int i = 0; !stoken.stop_requested(); i++) {
        if (i == n - 1) i = 0;
        g.add_edge(i, i + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    ConcurrentGraph graph(N);

    SafePrint() << "--- START TESTÓW ---\n";

    std::vector<std::jthread> builders, destroyers;
    for (int i = 0; i < 2; i++) {
        builders.emplace_back(choas_builder, std::ref(graph));
        destroyers.emplace_back(chaos_destroyer, std::ref(graph)); 
    }

    std::jthread finder(path_finder, std::ref(graph));
    std::jthread sneaky(sneaky_builder, std::ref(graph));

    finder.join();

    SafePrint() << "[MAIN] Clean up - Finder found path\n";

    for (auto& b : builders) b.request_stop();
    for (auto& d : destroyers) d.request_stop();
    sneaky.request_stop();

    return 0;
}