#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include "Core/HAL/PlatformType.h"


template <typename T>
class TArray : protected std::vector<T>
{
public:
    // Iterator를 사용하기 위함
    using std::vector<T>::begin;
    using std::vector<T>::end;
    using std::vector<T>::rbegin;
    using std::vector<T>::rend;
    using std::vector<T>::operator[];

public:
    void Init(const T& Element, size_t Number);
    void Add(const T& Item);
    void AddUnique(const T& Item);
    void Emplace(T&& Item);
    void Empty();
    int32 Remove(const T& Item);
    bool RemoveSingle(const T& Item);
    void RemoveAt(int32 Index);
    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const T&>
    int32 RemoveAll(const Predicate& Pred);
    T* GetData();

    int32 Find(const T& Item);
    bool Find(const T& Item, int32& Index);

    /** Size */
    size_t Num() const;

    /** Capacity */
    size_t Len() const;

    void Sort();
    template <typename Compare>
        requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
    void Sort(const Compare& CompFn);
};

template <typename T>
void TArray<T>::Init(const T& Element, size_t Number)
{
    this->assign(Number, Element);
}

template <typename T>
void TArray<T>::Add(const T& Item)
{
    this->push_back(Item);
}

template <typename T>
void TArray<T>::AddUnique(const T& Item)
{
    if (Find(Item) == -1)
    {
        this->push_back(Item);
    }
}

template <typename T>
void TArray<T>::Emplace(T&& Item)
{
    this->emplace_back(std::move(Item));
}

template <typename T>
void TArray<T>::Empty()
{
    this->clear();
}

template <typename T>
int32 TArray<T>::Remove(const T& Item)
{
    auto oldSize = this->size();
    this->erase(std::remove(this->begin(), this->end(), Item), this->end());
    return static_cast<int32>(oldSize - this->size());
}

template <typename T>
bool TArray<T>::RemoveSingle(const T& Item)
{
    auto it = std::find(this->begin(), this->end(), Item);
    if (it != this->end())
    {
        this->erase(it);
        return true;
    }
    return false;
}

template <typename T>
void TArray<T>::RemoveAt(int32 Index)
{
    if (Index >= 0 && static_cast<size_t>(Index) < this->size())
    {
        this->erase(this->begin() + Index);
    }
}

template <typename T>
template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, const T&>
int32 TArray<T>::RemoveAll(const Predicate& Pred)
{
    auto oldSize = this->size();
    this->erase(std::remove_if(this->begin(), this->end(), Pred), this->end());
    return static_cast<int32>(oldSize - this->size());
}

template <typename T>
T* TArray<T>::GetData()
{
    return this->data();
}

template <typename T>
int32 TArray<T>::Find(const T& Item)
{
    const auto it = std::find(this->begin(), this->end(), Item);
    return it != this->end() ? std::distance(this->begin(), it) : -1;
}

template <typename T>
bool TArray<T>::Find(const T& Item, int32& Index)
{
    Index = Find(Item);
    return (Index != -1);
}

template <typename T>
size_t TArray<T>::Num() const
{
    return this->size();
}

template <typename T>
size_t TArray<T>::Len() const
{
    return this->capacity();
}

template <typename T>
void TArray<T>::Sort()
{
    std::sort(this->begin(), this->end());
}

template <typename T>
template <typename Compare>
    requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
void TArray<T>::Sort(const Compare& CompFn)
{
    std::sort(this->begin(), this->end(), CompFn);
}
