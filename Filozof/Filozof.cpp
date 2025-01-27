#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <semaphore>
#include <thread>

int N;
enum class State {
    THINKING,
    HUNGRY,
    EATING
};
int inline left(const int i) {
    return (i - 1 + N) % N;
}

int inline right(const int i) {
    return (i + 1) % N;
}
struct default_binary_semaphore
{
    std::binary_semaphore sem;

    default_binary_semaphore() : sem (std::binary_semaphore{0}) {}
    explicit default_binary_semaphore(auto count) : sem(count) {}
};

std::vector<State> STATE;

std::mutex CRITICAL_REGION_MTX;
std::mutex OUTPUT_MTX;

std::vector<default_binary_semaphore> BOTH_FORKS_AVAILABLE;

int rand(const int min, const int max) {
    static std::mt19937 rnd(std::random_device{}());
    return std::uniform_int_distribution(min, max)(rnd);
}

void checkForks(const int i) {
    if(STATE[i] == State::HUNGRY &&
        STATE[left(i)] != State::EATING &&
        STATE[right(i)] != State::EATING) {
        STATE[i] = State::EATING;
        BOTH_FORKS_AVAILABLE[i].sem.release();
    }
}

void think(const int i) {
    const int duration = rand(3, 8);
    {
        std::lock_guard lk(OUTPUT_MTX);
        std::cout << i << " is thinking\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(duration));
}

void takeForks(const int i) {
    {
        std::lock_guard lk{CRITICAL_REGION_MTX};
        STATE[i] = State::HUNGRY;
        {
            std::lock_guard lkout(OUTPUT_MTX);
            std::cout << i << " is hungry\n";
        }
        checkForks(i);
    }
    BOTH_FORKS_AVAILABLE[i].sem.acquire();
}

void eat(const int i) {
    const int duration = rand(3, 8);
    {
        std::lock_guard lk(OUTPUT_MTX);
        std::cout << i << " is eating\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(duration));
}

void put_forks(const int i) {

    std::lock_guard lk{CRITICAL_REGION_MTX};
    STATE[i] = State::THINKING;
    checkForks(left(i));
    checkForks(right(i));
}

[[noreturn]] void philosopher(const int i) {
    while(true) {
        think(i);
        takeForks(i);
        eat(i);
        put_forks(i);
    }
}
bool isNumber(const std::string& s)
{
    auto it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int main(int argc, char *argv[]) {
    if(argc != 2 && isNumber(argv[1])) {
        std::cout << "Usage: " << argv[1] << " <number of philosophers>\n";
        return EXIT_FAILURE;
    }
    N = std::stoi(argv[1]);
    std::vector<std::jthread> threads;
    BOTH_FORKS_AVAILABLE = std::vector<default_binary_semaphore>(N);
    STATE = std::vector<State>(N, State::THINKING);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([i] { philosopher(i); });
    }
    for (int i = 0; i < N; ++i) {
        threads[i].join();
    }
}