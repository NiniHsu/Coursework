#ifndef OGL4CORE2_CORE_TRACKBALL_H
#define OGL4CORE2_CORE_TRACKBALL_H

#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OGL4Core2::Core {
    class Trackball {
    public:
        Trackball();

        glm::quat& getQ() { return q_; }

        [[nodiscard]] const glm::quat& getQ() const { return q_; }

        [[nodiscard]] glm::mat4 getRot() const;

        void reset();

        void track(float p1x, float p1y, float p2x, float p2y);

    private:
        [[nodiscard]] glm::vec3 mapToSphere(float x, float y) const;

        const float radius_;
        glm::quat q_;
    };
} // namespace OGL4Core2::Core

#endif // OGL4CORE2_CORE_TRACKBALL_H
