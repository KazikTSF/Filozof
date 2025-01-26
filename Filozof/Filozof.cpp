#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <semaphore>
#include <thread>

constexpr int N = 5;
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

State STATE[N];

std::mutex CRITICAL_REGION_MTX;
std::mutex OUTPUT_MTX;
std::binary_semaphore BOTH_FORKS_AVAILABLE[N]
{
    std::binary_semaphore{0}, std::binary_semaphore{0},
    std::binary_semaphore{0}, std::binary_semaphore{0},
    std::binary_semaphore{0}
};

int rand(const int min, const int max) {
    static std::mt19937 rnd(std::random_device{}());
    return std::uniform_int_distribution(min, max)(rnd);
}

void checkForks(const int i) {
    if(STATE[i] == State::HUNGRY &&
        STATE[left(i)] != State::EATING &&
        STATE[right(i)] != State::EATING) {
        STATE[i] = State::EATING;
        BOTH_FORKS_AVAILABLE[i].release();
    }
}

void think(const int i) {
    {
        std::lock_guard lk(OUTPUT_MTX);
        std::cout << i << " is thinking\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(4));
}

void takeForks(const int i) {
    {
        std::lock_guard lk{CRITICAL_REGION_MTX};
        STATE[i] = State::HUNGRY;
        {
            std::lock_guard lkout(OUTPUT_MTX);
            std::cout << i << " is State::HUNGRY\n";
        }
        checkForks(i);
    }
    BOTH_FORKS_AVAILABLE[i].acquire();
}

void eat(const int i) {
    {
        std::lock_guard lk(OUTPUT_MTX);
        std::cout << i << " is eating\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
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
//     if(argc != 1 && isNumber(argv[0])) {
//         std::cout << "Usage: " << argv[0] << " <number of philosophers>\n";
//     }
//     int n = std::stoi(argv[0]);
    
    std::jthread t0([&] { philosopher(0); });
    std::jthread t1([&] { philosopher(1); });
    std::jthread t2([&] { philosopher(2); });
    std::jthread t3([&] { philosopher(3); });
    std::jthread t4([&] { philosopher(4); });
}