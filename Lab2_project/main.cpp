#define GLM_ENABLE_EXPERIMENTAL  // Enable experimental features

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include <string>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include <thread>
#include <chrono>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir_d;   // directional light
glm::vec3 lightColor_d;
glm::vec3 lightDir_p1;   // point light 1
glm::vec3 lightColor_p1;
glm::vec3 lightPosEye_p1;
glm::vec3 lightPosEye_p2;
glm::vec3 lightPosEye_p3;
glm::vec3 lightPosEye_p4;
GLfloat constant_p1 = 1.0f;
GLfloat linear_p1 = 0.7f;
GLfloat quadratic_p1 = 1.8f;
GLint pointlights_on = 1;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc_d;    // directional light
GLint lightColorLoc_d;
GLint lightDirLoc_p1;   // point light 1
GLint lightColorLoc_p1;
GLint lightPosEyeLoc_p1;
GLint lightPosEyeLoc_p2;
GLint lightPosEyeLoc_p3;
GLint lightPosEyeLoc_p4;
GLint constantLoc_p1;
GLint linearLoc_p1;
GLint quadraticLoc_p1;
GLint pointlights_onLoc;

glm::vec3 cameraPositionInit = glm::vec3(12.2f, 0.88f, 1.997f);
glm::vec3 cameraTargetInit = glm::vec3(-30.66f, 0.558f, -0.607f);
glm::vec3 cameraUpInit = glm::vec3(0.0f, 1.0f, 0.0f);

// camera
gps::Camera myCamera(
    cameraPositionInit,
    cameraTargetInit,
    cameraUpInit);

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D world;
GLfloat angle = 0;

// shaders
gps::Shader myBasicShader;
gps::Shader depthMapShader;

// shadow 
const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;
GLuint shadowMapFBO;
GLuint depthMapTexture;


bool firstMouseMove = true;
double prevX, prevY = 0;

bool lKeyReleased = true;
bool tKeyReleased = true;
bool tour_play = false;
enum ANIMATION_STATE {A_INIT, A_MOVE1, A_MOVE2, A_MOVE3, A_MOVE4, A_MOVE5, A_MOVE6, A_DONE};
ANIMATION_STATE animation_state = A_INIT;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    //TODO
    WindowDimensions wd;
    glfwGetFramebufferSize(myWindow.getWindow(), &wd.width, &wd.height);
    myWindow.setWindowDimensions(wd);

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)wd.width / (float)wd.height,
        0.1f, 50.0f);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glViewport(0, 0, wd.width, wd.height);
}

int line = 0;
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        if (line % 3 == 1) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            line++;
        }
        else if (line % 3 == 2) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            line++;
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            line++;
        }

    }
}

double sensitivity = 0.5f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (!tour_play) {
        if (firstMouseMove) {
            glm::mat4 auxMatrix = myCamera.getViewMatrix();
            myCamera.setYaw(-glm::degrees(atan2(-auxMatrix[0][2], auxMatrix[2][2])));
            myCamera.setPitch(-glm::degrees(asin(auxMatrix[1][2])));
            prevX = xpos;
            prevY = ypos;
            firstMouseMove = false;
        }
        else {
            double offsetX = prevX - xpos;
            double offsetY = prevY - ypos;
            prevX = xpos;
            prevY = ypos;

            offsetX *= sensitivity;
            offsetY *= sensitivity;

            printf("Mouse: %lf, %lf\n", offsetX, offsetY);
            myCamera.rotate(offsetY, offsetX);
            view = myCamera.getViewMatrix();
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        }
    }
}

int a_iter = 0;
void start_animation() {
    switch (animation_state) {
    case A_INIT: 
        myCamera.initialize(cameraPositionInit, cameraTargetInit, cameraUpInit);
        angle = 0;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        animation_state = A_MOVE1;
        break;
    case A_MOVE1:
        if (a_iter < 30) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            myCamera.rotate(0.0f, -0.5f);
            view = myCamera.getViewMatrix();
            a_iter++;
        }else {
            a_iter = 0;
            animation_state = A_MOVE2;
        }
        break;
    case A_MOVE2:
        if (a_iter < 30) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            view = myCamera.getViewMatrix();
            a_iter++;
        }else {
            a_iter = 0;
            animation_state = A_MOVE3;
        }
        break;
    case A_MOVE3:
        if (a_iter < 40) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            myCamera.rotate(0.0f, 0.45f);
            view = myCamera.getViewMatrix();
            a_iter++;
        }
        else {
            a_iter = 0;
            animation_state = A_MOVE4;
        }
        break;
    case A_MOVE4:
        if (a_iter < 55) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            myCamera.rotate(0.0f, -0.7f);
            view = myCamera.getViewMatrix();
            a_iter++;
        }
        else {
            a_iter = 0;
            animation_state = A_MOVE5;
        }
        break;
    case A_MOVE5:
        if (a_iter < 45) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            view = myCamera.getViewMatrix();
            a_iter++;
        }
        else {
            a_iter = 0;
            animation_state = A_MOVE6;
        }
        break;
    case A_MOVE6:
        if (a_iter < 31) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            myCamera.move(gps::MOVE_UP, cameraSpeed);
            myCamera.rotate(-0.5f, 6.0f);
            view = myCamera.getViewMatrix();
            a_iter++;
        }
        else {
            a_iter = 0;
            animation_state = A_DONE;
        }
        break;
    default:
        break;
    }
}

float xelem = 1.261f;
float yelem = 0.965f;
float zelem = -1.0f;
void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

    if (pressedKeys[GLFW_KEY_Q] && !tour_play) {
        angle -= 1.0f;
        // update model matrix for object
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E] && !tour_play) {
        angle += 1.0f;
        // update model matrix for object
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_UP]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for object
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_L]) {
        if (lKeyReleased) {
            pointlights_on = 1 - pointlights_on;
            pointlights_onLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointlights_on");
            glUniform1i(pointlights_onLoc, pointlights_on);
            lKeyReleased = false;
        }        
    }
    else {
        lKeyReleased = true;
    }

    if (pressedKeys[GLFW_KEY_T]) {
        if (tKeyReleased) {
            if(animation_state == A_INIT){
                tour_play = true;
                start_animation();
            }
        }
    }
    else {
        tKeyReleased = true;
        if (animation_state == A_DONE) {
            animation_state = A_INIT;
            tour_play = false;
        }
    }
    if (pressedKeys[GLFW_KEY_Z]) {
        xelem += 0.001f;
        lightDir_d = glm::vec3(xelem, yelem, zelem);
        //lightDirLoc_d = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir_d");
        glUniform3fv(lightDirLoc_d, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir_d));
    }

}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    world.LoadModel("models/myWorld/world.obj");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    depthMapShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for object
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for object
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir_d = glm::vec3(1.261f, 0.965f, -1.0f);
	lightDirLoc_d = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir_d");
	glUniform3fv(lightDirLoc_d, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir_d));

	//set light color
	lightColor_d = glm::vec3(0.051f, 0.055f, 0.141f); //blue light
	lightColorLoc_d = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor_d");
	glUniform3fv(lightColorLoc_d, 1, glm::value_ptr(lightColor_d));

    //set the light direction
    lightDir_p1 = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir_p1");
    glUniform3fv(lightDirLoc_p1, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir_p1));

    //set light color
    lightColor_p1 = glm::vec3(1.0f, 1.0f, 0.0f); //yellow light
    lightColorLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor_p1");
    glUniform3fv(lightColorLoc_p1, 1, glm::value_ptr(lightColor_p1));

    //set position of positional light 
    lightPosEye_p1 = glm::vec3(-5.297f, 2.4183f, -5.9149f);
    lightPosEyeLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosEye_p1");
    glUniform3fv(lightPosEyeLoc_p1, 1, glm::value_ptr(lightPosEye_p1));

    lightPosEye_p2 = glm::vec3(-0.32386f, 2.4183f, 1.1181);
    lightPosEyeLoc_p2 = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosEye_p2");
    glUniform3fv(lightPosEyeLoc_p2, 1, glm::value_ptr(lightPosEye_p2));

    lightPosEye_p3 = glm::vec3(7.1655f, 2.4183f, -0.37917f);
    lightPosEyeLoc_p3 = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosEye_p3");
    glUniform3fv(lightPosEyeLoc_p3, 1, glm::value_ptr(lightPosEye_p3));

    lightPosEye_p4 = glm::vec3(14.719f, 2.4183f, 6.1155f);
    lightPosEyeLoc_p4 = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosEye_p4");
    glUniform3fv(lightPosEyeLoc_p4, 1, glm::value_ptr(lightPosEye_p4));

    constantLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "constant_p1");
    glUniform1f(constantLoc_p1, constant_p1);

    linearLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "linear_p1");
    glUniform1f(linearLoc_p1, linear_p1);

    quadraticLoc_p1 = glGetUniformLocation(myBasicShader.shaderProgram, "quadratic_p1");
    glUniform1f(quadraticLoc_p1, quadratic_p1);

    pointlights_onLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointlights_on");
    glUniform1i(pointlights_onLoc, pointlights_on);
}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    glm::mat4 lightView = glm::lookAt(lightDir_d, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = -40.0f, far_plane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void renderObject(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();

    if (!depthPass) {
        //send object model matrix data to shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        //send object normal matrix data to shader
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // draw object
    world.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (tour_play) {
        start_animation();
    }

    depthMapShader.useShaderProgram();

    glGetUniformLocation(depthMapShader.shaderProgram, "model");
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));


    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    renderObject(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

    glClear(GL_COLOR_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    //render the scene
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
	// render the object
	renderObject(myBasicShader, false);

}

void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    initOpenGLState();
    initFBO();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
