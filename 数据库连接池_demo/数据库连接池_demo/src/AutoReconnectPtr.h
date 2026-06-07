
#ifndef AUTORECONNECTPTR_H
#define AUTORECONNECTPTR_H

#include <memory>
#include <functional>

template<typename T>
class AutoReconnectPtr {
public:
    using ConnectionPtr = std::unique_ptr<T, std::function<void(T*)>>;
    
    explicit AutoReconnectPtr(std::function<ConnectionPtr()> getter)
        : m_getter(std::move(getter)), m_ptr(m_getter()) {}
    
    AutoReconnectPtr(const AutoReconnectPtr&) = delete;
    AutoReconnectPtr& operator=(const AutoReconnectPtr&) = delete;
    
    AutoReconnectPtr(AutoReconnectPtr&& other) noexcept
        : m_getter(std::move(other.m_getter)), m_ptr(std::move(other.m_ptr)) {}
    
    AutoReconnectPtr& operator=(AutoReconnectPtr&& other) noexcept {
        if (this != &other) {
            m_getter = std::move(other.m_getter);
            m_ptr = std::move(other.m_ptr);
        }
        return *this;
    }
    
    ~AutoReconnectPtr() = default;
    
    T* operator->() {
        ensureValid();
        return m_ptr.get();
    }
    
    T& operator*() {
        ensureValid();
        return *m_ptr.get();
    }
    
    bool isValid() const {
        return m_ptr && m_ptr->isValid();
    }

private:
    void ensureValid() {
        if (!m_ptr || !m_ptr->isValid()) {
            m_ptr = m_getter();
        }
    }
    
    std::function<ConnectionPtr()> m_getter;
    ConnectionPtr m_ptr;
};

#endif // AUTORECONNECTPTR_H
