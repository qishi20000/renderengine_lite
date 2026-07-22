#pragma once
#include <cstdint>
#include <functional>

namespace rel {

// A lightweight, generation-checked handle identifying a "thing that can
// have components" in the scene. This is intentionally NOT a full ECS —
// see EntityManager for details on why.
struct Entity {
    uint32_t id = 0;

    constexpr bool isNull() const noexcept { return id == 0; }
    constexpr bool operator==(const Entity& o) const noexcept { return id == o.id; }
    constexpr bool operator!=(const Entity& o) const noexcept { return id != o.id; }
};

constexpr Entity kNullEntity{0};

} // namespace rel

namespace std {
template<> struct hash<rel::Entity> {
    size_t operator()(const rel::Entity& e) const noexcept {
        return std::hash<uint32_t>()(e.id);
    }
};
} // namespace std
