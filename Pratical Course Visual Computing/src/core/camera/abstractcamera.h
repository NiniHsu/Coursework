#ifndef OGL4CORE2_CORE_ABSTRACTCAMERA_H
#define OGL4CORE2_CORE_ABSTRACTCAMERA_H

namespace OGL4Core2::Core {
    class AbstractCamera {
    public:
        enum class MouseControlMode {
            None = 0,
            Left = 1,
            Middle = 2,
            Right = 3,
        };

        virtual ~AbstractCamera() = default;

        virtual void mouseMoveControl(MouseControlMode mode, double oldX, double oldY, double newX, double newY) = 0;
        virtual void mouseScrollControl(double xoffset, double yoffset) = 0;
    };
} // namespace OGL4Core2::Core

#endif // OGL4CORE2_CORE_ABSTRACTCAMERA_H
