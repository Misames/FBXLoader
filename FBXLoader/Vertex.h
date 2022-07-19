#include <glm.hpp>

struct Vertex
{
    glm::vec3 position; //  3x4 octets = 12
    glm::vec3 normal;   // +3x4 octets = 24
    glm::vec2 uv;       // +2x4 octets = 32
    glm::vec4 tangent;  // +4x4 octets = 48

    static constexpr float EPSILON = 0.001f;

    static inline bool IsSame(const glm::vec2 &lhs, const glm::vec2 &rhs)
    {
        if (fabsf(lhs.x - rhs.x) < EPSILON && fabsf(lhs.y - rhs.y) < EPSILON)
            return true;
        return false;
    }

    static inline bool IsSame(const glm::vec3 &lhs, const glm::vec3 &rhs)
    {
        if (fabsf(lhs.x - rhs.x) < EPSILON && fabsf(lhs.y - rhs.y) < EPSILON && fabsf(lhs.z - rhs.z) < EPSILON)
            return true;
        return false;
    }

    inline bool IsSame(const Vertex &v) const
    {
        if (IsSame(position, v.position) && IsSame(normal, v.normal) && IsSame(uv, v.uv))
            return true;
        return false;
    }
};
