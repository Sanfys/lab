
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>
#include <chrono>
#include <utility>
#include <windows.h>
using namespace std;

// ============================================================
// 1.2.1: потоки без join() і без detach()
// ============================================================
namespace task1 {
void Thread1() { cout << 1 << endl; }
void Thread2() { cout << 2 << endl; }

void run() {
    thread t1(Thread1);
    thread t2(Thread2);

}
}

// ============================================================
// 1.2.2: потоки з detach()
// ============================================================
namespace task2 {
void Thread1() { cout << 1 << endl; }
void Thread2() { cout << 2 << endl; }

void run() {
    thread t1(Thread1);
    thread t2(Thread2);
    t1.detach();
    t2.detach();

}
}

// ============================================================
// 1.2.3: спільний список без синхронізації (гонка даних!)
// ============================================================
namespace task3 {
list<int> l;

void AddToList(int value) {
    for (int i = 0; i < 10; i++) {
        l.push_back(value + i);
        cout << "Додано елемент " << value + i << endl;
    }
}

void ListContains(int value) {
    for (int i = 0; i < 10; i++) {
        bool found = find(l.begin(), l.end(), value) != l.end();
        cout << "Елемент " << value << (found ? " входить" : " не входить") << " у список" << endl;
    }
}

void run() {
    l.clear();
    int start = 1, searched = 5;
    thread t1(AddToList, start);
    thread t2(ListContains, searched);
    t1.join();
    t2.join();
}
}

// ============================================================
// 1.2.4: ізолювання доступу до списку за допомогою mutex (lock/unlock)
// ============================================================
namespace task4 {
list<int> l;
mutex m;

void AddToList(int value) {
    for (int i = 0; i < 10; i++) {
        m.lock();
        l.push_back(value + i);
        cout << "Додано елемент " << value + i << endl;
        m.unlock();
    }
}

void ListContains(int value) {
    for (int i = 0; i < 10; i++) {
        m.lock();
        bool found = find(l.begin(), l.end(), value) != l.end();
        cout << "Елемент " << value << (found ? " входить" : " не входить") << " у список" << endl;
        m.unlock();
    }
}

void run() {
    l.clear();
    int start = 1, searched = 5;
    thread t1(AddToList, start);
    thread t2(ListContains, searched);
    t1.join();
    t2.join();
}
}

// ============================================================
// 1.2.5: по одній операції, 10 запусків кожного потоку (detach), lock_guard
// ============================================================
namespace task5 {
list<int> l;
mutex m;

void AddToList(int value) {
    lock_guard<mutex> guard(m);
    l.push_back(value);
    cout << "Додано елемент " << value << endl;
}

void ListContains(int value) {
    lock_guard<mutex> guard(m);
    bool found = find(l.begin(), l.end(), value) != l.end();
    cout << "Елемент " << value << (found ? " входить" : " не входить") << " у список" << endl;
}

void run() {
    {
        lock_guard<mutex> guard(m);
        l.clear();
    }
    int start = 1, searched = 5;
    for (int i = 0; i < 10; i++) {
        thread t1(AddToList, start + i);   // щоразу на 1 більше
        thread t2(ListContains, searched); // щоразу те саме число
        t1.detach();
        t2.detach();
    }

    this_thread::sleep_for(chrono::seconds(1));
}
}

// ============================================================
// 1.2.6 і 1.2.7: exchangePerson
//   6: Swap через std::lock + lock_guard(adopt_lock)
//   7: Swap через std::unique_lock + std::defer_lock
// ============================================================
namespace persons {
class someData {
public:
    string name, surname, address;
    int age = 0;
    someData() = default;
    someData(string n, string s, string a, int ag)
        : name(move(n)), surname(move(s)), address(move(a)), age(ag) {}
    void print() const {
        cout << name << " " << surname << ", " << address << ", вік: " << age << endl;
    }
};

class exchangePerson {
private:
    someData data;
    mutex m;

    static void swapAndPrint(exchangePerson& a, exchangePerson& b) {
        cout << "До обміну:" << endl;
        cout << "  p1: "; a.data.print();
        cout << "  p2: "; b.data.print();
        swap(a.data, b.data);
        cout << "Після обміну:" << endl;
        cout << "  p1: "; a.data.print();
        cout << "  p2: "; b.data.print();
    }

public:
    static void JohnDoe(exchangePerson& p) {
        lock_guard<mutex> guard(p.m);
        p.data = someData("John", "Doe", "Unknown", 120);
    }
    static void JacobSmith(exchangePerson& p) {
        lock_guard<mutex> guard(p.m);
        p.data = someData("Jacob", "Smith", "Known", 1);
    }

    // 1.2.6: std::lock + lock_guard(adopt_lock)
    static void Swap(exchangePerson& a, exchangePerson& b) {
        if (&a == &b) return; 
        lock(a.m, b.m);  
        lock_guard<mutex> la(a.m, adopt_lock);
        lock_guard<mutex> lb(b.m, adopt_lock);
        swapAndPrint(a, b);
    }

    // 1.2.7: std::unique_lock + std::defer_lock
    static void SwapUnique(exchangePerson& a, exchangePerson& b) {
        if (&a == &b) return;
        unique_lock<mutex> la(a.m, defer_lock);
        unique_lock<mutex> lb(b.m, defer_lock);
        lock(la, lb);
        swapAndPrint(a, b);
    }
};

template <typename SwapFn>
void runWith(SwapFn swapFn) {
    exchangePerson p1, p2;
    thread t1(exchangePerson::JohnDoe, ref(p1));
    thread t2(exchangePerson::JacobSmith, ref(p2));
    t1.detach();
    t2.detach();

    this_thread::sleep_for(chrono::milliseconds(200));

    thread t3(swapFn, ref(p1), ref(p2));
    t3.join();
}
}

namespace task6 {
void run() { persons::runWith(persons::exchangePerson::Swap); }
}

namespace task7 {
void run() { persons::runWith(persons::exchangePerson::SwapUnique); }
}

// ============================================================
// main: вибір завдання
// ============================================================
static void runTask(int n) {
    cout << "\n===== Завдання 1.2." << n << " =====" << endl;
    switch (n) {
        case 1: task1::run(); break;
        case 2: task2::run(); this_thread::sleep_for(chrono::milliseconds(200)); break;
        case 3: task3::run(); break;
        case 4: task4::run(); break;
        case 5: task5::run(); break;
        case 6: task6::run(); break;
        case 7: task7::run(); break;
        default: cout << "Невідомий номер завдання (1..7)" << endl;
    }
}

int main(int argc, char* argv[]) {
        SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    string choice;
    if (argc > 1) {
        choice = argv[1];
    } else {
        cout << "Оберіть завдання:\n"
                "  1 - потоки без join/detach (завершить програму через std::terminate!)\n"
                "  2 - потоки з detach\n"
                "  3 - список без синхронізації\n"
                "  4 - mutex lock/unlock\n"
                "  5 - detach + lock_guard\n"
                "  6 - exchangePerson: std::lock + adopt_lock\n"
                "  7 - exchangePerson: unique_lock + defer_lock\n"
                "  all - запустити 2..7\n"
                "Ваш вибір: ";
        cin >> choice;
    }

    if (choice == "all") {
        for (int n = 2; n <= 7; n++) runTask(n);
    } else {
        try {
            runTask(stoi(choice));
        } catch (...) {
            cout << "Некоректний ввід" << endl;
            return 1;
        }
    }
    return 0;
}
