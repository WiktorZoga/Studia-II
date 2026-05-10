#include <iostream>
#include <future>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <sstream>

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

class TaskExecutor {
    // Kolejka przechowująca wymazywane typowo zadania, które zwracają int
    std::deque<std::packaged_task<int()>> task_queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;
    
    // Infrastruktura wielowątkowa
    std::vector<std::thread> workers;
public:
    // Konstruktor startujący pule wątków egzekutora
    explicit TaskExecutor(size_t num_threads = 2) {
        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back(&TaskExecutor::executor_loop, this);
        }
    }

    // Destruktor upewniający się, że wątki zostaną bezpiecznie połączone
    ~TaskExecutor() {
        shutdown();
    }

    template<typename Func>
    std::future<int> submit_task(Func f) {
        // PUNKT 1:
        // 1. Utwórz std::packaged_task opakowujący 'f'
        // 2. Wydobądź powiązany future
        // 3. Bezpiecznie dodaj zadanie do 'task_queue' używając std::move
        // 4. Powiadom zmienną warunkową
        // 5. Zwróć future
        // --- MIEJSCE NA TWÓJ KOD ---
        std::packaged_task<int()> task(f);
        std::future<int> result = task.get_future();

        {
            std::lock_guard<std::mutex> lock(mtx);
            task_queue.emplace_back(std::move(task));
        }

        cv.notify_one();

        return result;
    }

    void executor_loop() {
        while (true) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] {return stop || !task_queue.empty(); });
                if (stop && task_queue.empty()) return;
                // PUNKT 1: Pobierz zadanie z kolejki (pamiętaj o semantyce przenoszenia!)
                // --- MIEJSCE NA TWÓJ KOD ---
                task = std::move(task_queue.front());
                task_queue.pop_front();
            }
            // PUNKT 1: Wykonaj pobrane zadanie.
            // PUNKT 2: Czy tutaj potrzebujemy try-catch chroniącego pętlę egzekutora
            // przed rzucanymi w zadaniach wyjątkami? Zaimplementuj odpowiednie podejście
            // i wyjaśnij je w komentarzu.
            // --- MIEJSCE NA TWÓJ KOD ---
            task(); 

            /* NIE POTRZEBUJEMY try-catch. 
               Mechanizm std::packaged_task ma wbudowaną obsługę wyjątków. 
               Jeśli zadanie rzuci wyjątek, task() przechwyci go i zapisze 
               we współdzielonym stanie (shared state). Wyjątek zostanie 
               ponownie rzucony dopiero w wątku, który 
               wywoła metodę .get() na powiązanym obiekcie std::future.
            */

        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return; // Zapobieganie wielokrotnemu wywoływaniu z destruktora
            stop = true;
        }
        cv.notify_all();
        for (auto& t: workers) {
            if (t.joinable()) t.join();
        }
    }
};

void diagnostic_worker() {
    // PUNKT 3:
    std::packaged_task<int()> health_check([]{
        SafePrint() << "[Diagnostyka] Pinging DB...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 200; // 200 OK
    });

    for (int i = 0; i < 3; i++) {
        // 1. Pobierz future (zwróć uwagę, że get_future można wywołać tylko raz dla bieżącego stanu)
        // 2. Wywołaj health_check bezpośrednio
        // 3. Wydrukuj wynik używając future.get()
        // 4. Skonfiguruj health_check do ponownego użycia w kolejnej iteracji pętli
        // --- MIEJSCE NA TWÓJ KOD ---
        std::future<int> result = health_check.get_future();
        health_check();
        std::cout << result.get() << "\n";
        health_check.reset();
    }
}

int main() {
    std::cout << "--- Start Systemu ---\n";
    TaskExecutor executor(2); // Inicjalizacja egzekutora dwoma wątkami wykonawczymi
    std::cout << "\nZlecanie zadan do egzekutora...\n";
    // Zlecanie poprawnie działającego zadania
    auto fut1 = executor.submit_task([] {
        SafePrint() << "[Zadanie 1] Zatwierdzanie transakcji A...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    });
    // Zlecanie zadania rzucającego wyjątek (weryfikacja PUNKTu 2)
    auto fut2 = executor.submit_task([] {
        SafePrint() << "[Zadanie 2] Zatwierdzanie transakcji B...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        throw std::runtime_error("DB Lockout");
        return 2;
    });
    // Odbiór wyników i sprawdzenie propagacji wyjątków
    try {
        if (fut1.valid())   SafePrint() << "Wynik zadania 1: " << fut1.get() << "\n";
    } catch (const std::exception& e) {
        SafePrint() << "Zadanie 1 zglosilo blad: " << e.what() << "\n";
    }

    try {
        if (fut2.valid())   SafePrint() << "Wynik zadania 2: " << fut2.get() << "\n";
    } catch (const std::exception& e) {
        SafePrint() << "Zadanie 2 zglosilo wyjatek (OCZEKIWANE zachowanie w Etapie 2): " << e.what() << "\n";
    }

    // Test PUNKTu 3 (Diagnostyka z użyciem reset)
    SafePrint() << "\n--- Uruchamianie testu diagnostycznego ---\n";
    std::thread diag_thread(diagnostic_worker);
    diag_thread.join();

    // Ręczne wyłączenie egzekutora (opcjonalne - odpali się też w destruktorze)
    executor.shutdown();
    std::cout << "\n--- Koniec Systemu ---\n";
    return 0;
}