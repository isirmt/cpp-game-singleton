#include <iostream>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

class GameObject;

// 基底クラス
class Component {
   public:
    virtual ~Component() = default;
    virtual void Update(GameObject& obj) {}
    virtual void Render(GameObject& obj) {}
};

class GameObject {
   private:
    std::unordered_map<std::type_index, std::shared_ptr<Component>> components;

   public:
    std::string name;

    GameObject(std::string _name) : name(_name) {}

    template <typename T, typename... Args>
    void AddComponent(Args&&... args) {
        components[typeid(T)] =
            std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    T* GetComponent() {
        auto it = components.find(typeid(T));
        return (it != components.end()) ? dynamic_cast<T*>(it->second.get())
                                        : nullptr;
    }

    void Update() {
        for (auto& [type, component] : components) {
            component->Update(*this);
        }
    }

    void Render() {
        for (auto& [type, component] : components) {
            component->Render(*this);
        }
    }
};

// 派生コンポーネント: 座標・移動
class TransformComponent : public Component {
   public:
    // 公開
    int x, y;

    TransformComponent(int x0, int y0) : x(x0), y(y0) {}

    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }
};

// 派生コンポーネント: 自動直線移動
class AutoLinearMovingComponent : public Component {
   public:
    int dx, dy;

    AutoLinearMovingComponent(int _dx, int _dy) : dx(_dx), dy(_dy) {}

    void Update(GameObject& obj) override {
        TransformComponent* transform = obj.GetComponent<TransformComponent>();
        if (transform) {
            transform->move(dx, dy);
        }
    }
};

// 派生コンポーネント: GameObjectからTransformを取得して描画するコンポーネント
class RendererComponent : public Component {
    void Render(GameObject& obj) override {
        TransformComponent* transform = obj.GetComponent<TransformComponent>();
        if (transform) {
            std::cout << "[" << obj.name << "] (" << transform->x
                      << ", " << transform->y << ")" << std::endl;
        }
    }
};

class SingletonApplication {
   private:
    static std::unique_ptr<SingletonApplication> pinstace_;
    static std::mutex mutex_;

    /* ゲーム用記述 */

    bool running;
    std::vector<std::shared_ptr<GameObject>> objects;

   protected:
    SingletonApplication() : running(true) {
        std::cout << "Application Initialized" << std::endl;
    }

   public:
    SingletonApplication(const SingletonApplication&) = delete;
    SingletonApplication& operator=(const SingletonApplication&) = delete;

    static SingletonApplication& GetInstance() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pinstace_) {
            pinstace_ = std::unique_ptr<SingletonApplication>(
                new SingletonApplication());
        }

        return *pinstace_;
    }

    /* ゲーム用記述 */

    void AddObject(std::shared_ptr<GameObject> obj) { objects.push_back(obj); }

    void Update() {
        for (auto& obj : objects) {
            obj->Update();
        }
    }

    void Render() {
        std::cout << "\033[2J\033[H";
        for (auto& obj : objects) {
            obj->Render();
        }
    }

    void Run() {
        while (running) {
            Update();
            Render();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void Stop() { running = false; }
};

std::unique_ptr<SingletonApplication> SingletonApplication::pinstace_ = nullptr;
std::mutex SingletonApplication::mutex_;

int main() {
    SingletonApplication& app = SingletonApplication::GetInstance();

    // シーンマネージャーを作ればシーン内のイニシャライズで定義できる

    // オブジェクト作成
    auto player = std::make_shared<GameObject>("Player");
    player->AddComponent<TransformComponent>(5, 5);
    player->AddComponent<RendererComponent>();

    auto enemy = std::make_shared<GameObject>("Enemy");
    enemy->AddComponent<TransformComponent>(10, 2);
    enemy->AddComponent<AutoLinearMovingComponent>(1, 2);
    enemy->AddComponent<RendererComponent>();

    // オブジェクト登録
    app.AddObject(player);
    app.AddObject(enemy);

    // メインループ
    app.Run();
}
