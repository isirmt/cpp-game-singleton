#include <iostream>
#include <memory>
#include <thread>

class Singleton {
   protected:
    Singleton(const int value) : value_(value) {}

    int value_;

   public:
    ~Singleton() {}

    // 複製禁止
    Singleton(Singleton& singleton) = delete;
    // 代入禁止
    void operator=(const Singleton&) = delete;

    static Singleton& GetInstance(const int& value = 1) {
        static Singleton instance(value);

        return instance;
    }

    int GetValue() const { return value_; }
};

// プロトタイプ
void ThreadFunc1();
void ThreadFunc2();

int main() {
    std::thread t1(ThreadFunc1);
    std::thread t2(ThreadFunc2);

    t1.join();
    t2.join();

    return 0;
}

void ThreadFunc1() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Singleton& singleton = Singleton::GetInstance(3);
    std::cout << singleton.GetValue() << std::endl;
}

void ThreadFunc2() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Singleton& singleton = Singleton::GetInstance(7);
    std::cout << singleton.GetValue() << std::endl;
}