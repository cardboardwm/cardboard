#ifndef __CARDBOARD_SAFEPOINTER_H_
#define __CARDBOARD_SAFEPOINTER_H_

#include <cassert>
#include <utility>

template <typename T>
class SafePointer {
public:
    explicit SafePointer() : ptr(nullptr) {}
    SafePointer(T& ref) : ptr(&ref) {}
    explicit SafePointer(T* ptr) : ptr(ptr) {}

    T& unwrap() {
        if (ptr == nullptr) {
            assert("bad access");
        }

        return *ptr;
    }

    const T& unwrap() const {
        if (ptr == nullptr) {
            assert("bad access");
        }

        return *ptr;
    }

    bool has_value() const noexcept {
        return ptr != nullptr;
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    bool operator==(const SafePointer<T>& other) const {
        return ptr == other.ptr;
    }

    bool operator!=(const SafePointer<T>& other) const {
        return ptr != other.ptr;
    }

    template<typename U, typename F>
    SafePointer<U> and_then(F&& f) {
        if (has_value()) {
            return f(unwrap());
        }

        return SafePointer<U>(nullptr);
    }

    template<typename F>
    void and_then(F&& f) {
        if (has_value()) {
            f(unwrap());
        }
    }

    template<typename F>
    SafePointer<T> or_else(F&& f) {
        if (!has_value()) {
            return f();
        }

        return *this;
    }

private:
    T* ptr;
};

template<typename T>
const auto NullPointer = SafePointer<T>(nullptr);

#endif // __CARDBOARD_SAFEPOINTER_H_
