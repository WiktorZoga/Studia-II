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
        vertices.reserve(size);
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
};

class SafePrint {
public:
    SafePrint() = default;

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

void slow_reader(ConcurrentGraph& g) {
    SafePrint() << "[SLOW] Czekam na statystyki...\n";
    g.print_node_stats(0);
    SafePrint() << "[SLOW] Koniec.\n";
}

void fast_reader(ConcurrentGraph& g) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
    
    SafePrint() << "[FAST] Sprawdzam are_connected(0, 1)...\n";
    auto start = std::chrono::steady_clock::now();
    
    bool conn = g.are_connected(0, 1);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    SafePrint() << "[FAST] Skonczyłem w " << diff << " ms. Wynik: " << conn << "\n";
}

void writer(ConcurrentGraph& g) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    
    SafePrint() << "[WRITER] Probuję dodac krawędzie...\n";
    auto start = std::chrono::steady_clock::now();
    
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(3, 4);
    g.add_edge(0, 3);
    g.add_edge(1, 4);
    g.add_edge(2, 4);
    g.add_edge(0, 4);

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    SafePrint() << "[WRITER] Koniec. Czekał: " << diff << " ms.\n";
}

int main() {
    ConcurrentGraph graph(5);

    SafePrint() << "START\n";

    std::jthread t1(slow_reader, std::ref(graph));
    std::jthread t3(writer, std::ref(graph));
    std::jthread t2(fast_reader, std::ref(graph)); 

    return 0;
}