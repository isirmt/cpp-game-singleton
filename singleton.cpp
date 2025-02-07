#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class Singleton {
   private:
    static std::unique_ptr<Singleton> pinstance_;
    static std::mutex mutex_;

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
        // 排他制御を実現する
        std::lock_guard<std::mutex> lock(mutex_);

        if (!pinstance_) {
            pinstance_ = std::unique_ptr<Singleton>(new Singleton(value));
        }

        return *pinstance_;
    }

    int GetValue() const { return value_; }
};

// 静的メンバの宣言
std::unique_ptr<Singleton> Singleton::pinstance_ = nullptr;
std::mutex Singleton::mutex_;

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
    Singleton& singleton = Singleton::GetInstance(3);
    std::cout << singleton.GetValue() << std::endl;
}

void ThreadFunc2() {
    Singleton& singleton = Singleton::GetInstance(7);
    std::cout << singleton.GetValue() << std::endl;
}