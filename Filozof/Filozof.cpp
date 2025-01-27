#include <chrono>
#include <iostream>
#include <mutex>
#include <ncurses.h>
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
    const auto text = std::to_string(i+1).append("\tis thinking [\n");
    const auto tabLen = 7-std::to_string(i+1).size();
    const int duration = rand(5000, 10000);
    {
        std::lock_guard lk(OUTPUT_MTX);
        attron(COLOR_PAIR(1));
        mvprintw(i,0,text.c_str());
        mvaddch(i, tabLen + text.size()+19, ']');
        attroff(COLOR_PAIR(1));
        refresh();
    }
    for (int j = 0; j < 20; ++j) {
        {
            std::lock_guard lk(OUTPUT_MTX);
            attron(COLOR_PAIR(1));
            mvaddch(i, tabLen + text.size() + j-1, '#');
            attroff(COLOR_PAIR(1));
            refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(duration/20));
    }
}

void takeForks(const int i) {
    {
        std::lock_guard lk{CRITICAL_REGION_MTX};
        STATE[i] = State::HUNGRY;
        {
            std::lock_guard lkOut(OUTPUT_MTX);
            attron(COLOR_PAIR(3));
            mvprintw(i,0,std::to_string(i+1).append("\tis hungry\n").c_str());
            attroff(COLOR_PAIR(3));
            refresh();
        }
        checkForks(i);
    }
    BOTH_FORKS_AVAILABLE[i].sem.acquire();
}

void eat(const int i) {
    const auto text = std::to_string(i+1).append("\tis eating   [\n");
    const auto tabLen = 7-std::to_string(i+1).size();
    const int duration = rand(5000, 10000);
    {
        std::lock_guard lk(OUTPUT_MTX);
        attron(COLOR_PAIR(2));
        mvprintw(i,0,text.c_str());
        mvaddch(i, tabLen + text.size()+19, ']');
        attroff(COLOR_PAIR(2));
        refresh();
    }
    for (int j = 0; j < 20; ++j) {
        {
            std::lock_guard lk(OUTPUT_MTX);
            attron(COLOR_PAIR(2));
            mvaddch(i, tabLen + text.size() + j-1, '#');
            attroff(COLOR_PAIR(2));
            refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(duration/20));
    }
}

void put_forks(const int i) {

    std::lock_guard lk{CRITICAL_REGION_MTX};
    STATE[i] = State::THINKING;
    checkForks(left(i));
    checkForks(right(i));
}

void philosopher(const int i) {
    for(int j = 0; j < 5; ++j) {
        think(i);
        takeForks(i);
        eat(i);
        put_forks(i);
    }
    mvprintw(i,0, "Finished\n");
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
    initscr();
    start_color();
    N = std::stoi(argv[1]);
    if (N < 5) {
        std::cout << "Number of philosophers must be greater than 4\n";
        return EXIT_FAILURE;
    }
    std::vector<std::jthread> threads;
    BOTH_FORKS_AVAILABLE = std::vector<default_binary_semaphore>(N);
    STATE = std::vector<State>(N, State::THINKING);
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    curs_set(0);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([i] { philosopher(i); });
    }
    for (int i = 0; i < N; ++i) {
        threads[i].join();
    }
    endwin();
    return 0;
}