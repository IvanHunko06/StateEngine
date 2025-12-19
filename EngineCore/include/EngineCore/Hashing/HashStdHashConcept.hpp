#include <functional>

template <typename T>
concept HasStdHash = requires(const T& obj) {
    { std::hash<T> {}(obj) } -> std::same_as<size_t>;
};