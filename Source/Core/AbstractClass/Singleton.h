#pragma once

/**
 * Singleton 객체 템플릿
 */
template <typename Derived>
class TSingleton
{
private:
    static Derived* Instance;
    TSingleton() = default;

    friend Derived;

public:
    ~TSingleton();

    // 이동 & 복사 생성자 제거
    TSingleton(const TSingleton&) = delete;
    TSingleton& operator=(const TSingleton&) = delete;
    TSingleton(TSingleton&&) = delete;
    TSingleton& operator=(TSingleton&&) = delete;

    static Derived& Get();
};


template <typename Derived>
TSingleton<Derived>::~TSingleton()
{
    delete Instance;
}

template <typename Derived>
Derived& TSingleton<Derived>::Get()
{
    if (Instance == nullptr)
    {
        Instance = new Derived();
    }
    return *Instance;
}

template <typename Derived>
Derived* TSingleton<Derived>::Instance = nullptr;
