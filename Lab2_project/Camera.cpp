#include "Camera.hpp"
#include <glm/gtx/euler_angles.hpp>

namespace gps {

    void Camera::initialize(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp){
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraSceneUpDirection = cameraUp;
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = -glm::normalize(glm::cross((-this->cameraFrontDirection), this->cameraSceneUpDirection));
        this->cameraUpDirection = glm::cross(-this->cameraFrontDirection, this->cameraRightDirection);

        //TODO - Update the rest of camera parameters
        glm::mat4 auxMatrix = this->getViewMatrix();
        yaw = -glm::degrees(atan2(-auxMatrix[0][2], auxMatrix[2][2]));
        pitch = -glm::degrees(asin(auxMatrix[1][2]));
        /*yaw = -glm::degrees(asin(auxMatrix[1][2]));
        pitch = -glm::degrees(atan2(-auxMatrix[0][2], auxMatrix[2][2]));*/
    }

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->initialize(cameraPosition, cameraTarget, cameraUp);
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraSceneUpDirection);
    }

    void Camera::setYaw(float yaw) {
        this->yaw = yaw;
    }

    void Camera::setPitch(float pitch) {
        this->pitch = pitch;
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        if (direction == MOVE_FORWARD) {
            this->cameraPosition += cameraFrontDirection * speed;
        }
        else if (direction == MOVE_BACKWARD) {
            this->cameraPosition -= cameraFrontDirection * speed;
        }
        else if (direction == MOVE_LEFT) {
            this->cameraPosition -= cameraRightDirection * speed;
        }
        else if (direction == MOVE_RIGHT) {
            this->cameraPosition += cameraRightDirection * speed;
        }
        else if (direction == MOVE_UP) {
            this->cameraPosition += cameraUpDirection * speed;
        }
        else if (direction == MOVE_DOWN) {
            this->cameraPosition -= cameraUpDirection * speed;
        }
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis

    void Camera::rotate(float pitch, float yaw) {
        //TODO
        this->pitch += pitch;
        this->yaw += yaw; 

        glm::mat4 rotation = glm::yawPitchRoll(glm::radians(this->yaw), glm::radians(this->pitch), 0.0f);
        this->cameraFrontDirection = glm::normalize(rotation * glm::vec4(glm::vec3(0.0f, 0.0f, -1.0f), 0.0f));
        this->cameraRightDirection = -glm::normalize(glm::cross(-this->cameraFrontDirection, this->cameraSceneUpDirection));
        this->cameraUpDirection = glm::cross(-this->cameraFrontDirection, this->cameraRightDirection);
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }
}
