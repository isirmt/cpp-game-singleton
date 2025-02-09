#include <iostream>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

class GameObject;
class PChangeComponent;
class PResetComponent;

// 基底クラス
class Component {
   public:
    virtual ~Component() = default;
    virtual void Update(GameObject& obj) {}
    virtual void Render(GameObject& obj) {}
};

class GameObject {
   private:
    std::unordered_map<std::type_index, std::shared_ptr<Component>> components_;

   public:
    std::string name;

    GameObject(std::string _name) : name(_name) {}

    template <typename T, typename... Args>
    void AddComponent(Args&&... args) {
        components_[typeid(T)] =
            std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    std::shared_ptr<T> GetComponent() {
        auto it = components_.find(typeid(T));
        return (it != components_.end())
                   ? std::dynamic_pointer_cast<T>(it->second)
                   : nullptr;
    }

    void Update() {
        for (auto& [type, component] : components_) {
            component->Update(*this);
        }
    }

    void Render() {
        for (auto& [type, component] : components_) {
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
        auto transform = obj.GetComponent<TransformComponent>();
        if (transform) {
            transform->move(dx, dy);
        }
    }
};

// 派生コンポーネント: GameObjectからTransformを取得して描画するコンポーネント
class RendererComponent : public Component {
    void Render(GameObject& obj) override {
        auto transform = obj.GetComponent<TransformComponent>();
        if (transform) {
            std::cout << "[" << obj.name << "] (" << transform->x << ", "
                      << transform->y << ")" << std::endl;
        }
    }
};

class Scene {
   private:
    std::vector<std::shared_ptr<GameObject>> objects;

   public:
    Scene() = default;
    virtual ~Scene() {}

   public:
    virtual void Update() {
        for (auto& obj : objects) {
            obj->Update();
        }
    }

    virtual void Render() {
        for (auto& obj : objects) {
            obj->Render();
        }
    }

    virtual void Start() {}

    virtual void Reset() {
        objects.clear();
        Start();
    }

    void AddObject(std::shared_ptr<GameObject> obj) { objects.push_back(obj); }
};

class HomeScene : public Scene {
   public:
    HomeScene() {}

    void Start() override {
        std::cout << "### HomeScene ###" << std::endl;
        // オブジェクト作成
        auto player = std::make_shared<GameObject>("Player");
        player->AddComponent<TransformComponent>(5, 5);
        player->AddComponent<RendererComponent>();

        auto enemy = std::make_shared<GameObject>("Enemy");
        enemy->AddComponent<TransformComponent>(10, 2);
        enemy->AddComponent<AutoLinearMovingComponent>(1, 2);
        enemy->AddComponent<RendererComponent>();
        enemy->AddComponent<PChangeComponent>(15);

        AddObject(player);
        AddObject(enemy);
    }
};

class GameScene : public Scene {
   public:
    GameScene() {}

    void Start() override {
        std::cout << "### GameScene ###" << std::endl;
        // オブジェクト作成
        auto player = std::make_shared<GameObject>("Player");
        player->AddComponent<TransformComponent>(15, 4);
        player->AddComponent<RendererComponent>();

        auto enemy = std::make_shared<GameObject>("Enemy2");
        enemy->AddComponent<TransformComponent>(10, 2);
        enemy->AddComponent<AutoLinearMovingComponent>(0, 2);
        enemy->AddComponent<PResetComponent>(20);
        enemy->AddComponent<RendererComponent>();

        AddObject(player);
        AddObject(enemy);
    }
};

class SingletonApplication {
   private:
    static std::unique_ptr<SingletonApplication> pinstace_;
    static std::once_flag initFlag_;

    bool running;
    bool requestedReseting;
    std::shared_ptr<Scene> currentScene_;

   protected:
    SingletonApplication() : running(true), requestedReseting(false) {
        std::cout << "Application Initialized" << std::endl;
    }

   public:
    SingletonApplication(const SingletonApplication&) = delete;
    SingletonApplication& operator=(const SingletonApplication&) = delete;

    static SingletonApplication& GetInstance() {
        std::call_once(initFlag_, []() {
            pinstace_ = std::unique_ptr<SingletonApplication>(
                new SingletonApplication());
        });

        if (!pinstace_) {
            pinstace_ = std::unique_ptr<SingletonApplication>(
                new SingletonApplication());
        }

        return *pinstace_;
    }

    void ChangeScene(std::shared_ptr<Scene> newScene) {
        currentScene_ = std::move(newScene);
        currentScene_->Start();
    }

    void RequestResetingScene() { requestedReseting = true; }

    void Update() {
        if (currentScene_) {
            currentScene_->Update();

            if (requestedReseting) {
                requestedReseting = false;
                currentScene_->Reset();
            }
        }
    }

    void Render() {
        std::cout << "\033[2J\033[H";
        if (currentScene_) {
            currentScene_->Render();
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
std::once_flag SingletonApplication::initFlag_;

class PChangeComponent : public Component {
   public:
    int threshold;

    PChangeComponent(int _threshold) : threshold(_threshold) {}

    void Update(GameObject& obj) override {
        auto ptransform_ = obj.GetComponent<TransformComponent>();
        if (ptransform_) {
            if (ptransform_->x >= threshold || ptransform_->y >= threshold) {
                SingletonApplication::GetInstance().ChangeScene(
                    std::make_shared<GameScene>());
            }
        }
    }
};

class PResetComponent : public Component {
   public:
    int threshold;

    PResetComponent(int _threshold) : threshold(_threshold) {}

    void Update(GameObject& obj) override {
        auto ptransform_ = obj.GetComponent<TransformComponent>();
        if (ptransform_) {
            if (ptransform_->x >= threshold || ptransform_->y >= threshold) {
                SingletonApplication::GetInstance().RequestResetingScene();
            }
        }
    }
};

int main() {
    SingletonApplication& app = SingletonApplication::GetInstance();

    auto phomeScene = std::make_shared<HomeScene>();
    app.ChangeScene(phomeScene);

    // メインループ
    app.Run();
}
