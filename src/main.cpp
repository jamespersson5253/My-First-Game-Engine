#include "glad/gl.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include "GLFW/glfw3native.h"
#include "glm.hpp"
#include "ext.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

std::vector<float> verts;
std::vector<unsigned int> inds;

GladGLContext gl;

std::string getFile(std::string path) {
    std::ifstream f(path, std::ios::binary);
    if(!f) return std::string("");
    std::stringstream buf;
    buf << f.rdbuf();
    std::string fcon = buf.str();
    return fcon;
}

void formatMesh(const std::string& input) {
    std::stringstream ss(input);
    std::string line;

    verts.clear();
    inds.clear();

    while (std::getline(ss, line)) {
        std::stringstream ls(line);

        char type;
        ls >> type;

        if (type == 'v') {
            float x, y, z, r, g, b;
            char c;

            ls >> x >> y >> z >> c >> r >> g >> b;

            verts.insert(verts.end(), { x, y, z, r, g, b });
        }
        else if (type == 'i') {
            unsigned int a, b, c;

            ls >> a >> b >> c;

            inds.insert(inds.end(), { a, b, c });
        }
    }

    std::cout << "vert = {\n";

    for (size_t i = 0; i < verts.size(); i++) {
        std::cout << verts[i];

        if (i + 1 < verts.size())
            std::cout << ", ";

        if ((i + 1) % 6 == 0)
            std::cout << '\n';
    }

    std::cout << "}\n";
    std::cout << "ind = {";

    for (size_t i = 0; i < inds.size(); i++) {
        std::cout << inds[i];

        if (i + 1 < inds.size())
            std::cout << ", ";
    }

    std::cout << "}\n";
}

glm::mat4 proj = glm::perspective(
        glm::radians(90.f),
        640.f / 480.f,
        .1f, 100.f
);

void fbRszCallback(GLFWwindow* window, int width, int height) {
    gl.Viewport(0, 0, width, height);
    proj = glm::perspective(
        glm::radians(90.f),
        static_cast<float>(width) / static_cast<float>(height),
        .1f, 100.f
    );
}

glm::vec3 eyePos(0, 0, 1);
glm::vec3 legPos(0, -2, 1);
glm::vec3 dir;
float pitch = 0;
float yaw = -90;

float lastX = 320.0f;
float lastY = 240.0f;

bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xOffset = static_cast<float>(xpos) - lastX;
    float yOffset = lastY - static_cast<float>(ypos);

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.9f)
        pitch = 89.9f;

    if (pitch < -89.9f)
        pitch = -89.9f;

    float pitchR = glm::radians(pitch);
    float yawR = glm::radians(yaw);
    
    dir = glm::vec3(
        glm::cos(yawR) * glm::cos(pitchR),
        glm::sin(pitchR),
        glm::sin(yawR) * glm::cos(pitchR)
    );
}

int main() {
    if(!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        640, 
        480, 
        "Hello from OpenGL!", 
        nullptr, 
        nullptr
    );

    if(!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLContext(&gl, (GLADloadfunc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    glfwSetFramebufferSizeCallback(window, fbRszCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSwapInterval(1);

    std::string path("mesh.msh");
    formatMesh(getFile("meshes/"+path));

    GLuint VAO, VBO, EBO;
    GLuint buffers[2];
    gl.GenBuffers(2, buffers);
    VBO = buffers[0];
    EBO = buffers[1];
    gl.GenVertexArrays(1, &VAO);
    gl.BindVertexArray(VAO);
    gl.BindBuffer(GL_ARRAY_BUFFER, VBO);
    gl.BufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_DYNAMIC_DRAW
    );
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    gl.BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        inds.size() * sizeof(unsigned int),
        inds.data(),
        GL_DYNAMIC_DRAW
    );
    gl.VertexAttribPointer(
        0,
        3,
        GL_FLOAT, 
        GL_FALSE, 
        6*sizeof(float), 
        nullptr
    );
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(
        1, 
        3, 
        GL_FLOAT, 
        GL_FALSE, 
        6*sizeof(float),
        (void*)(3*sizeof(float))
    );
    gl.EnableVertexAttribArray(1);

    std::string vtSrcStr = getFile("shaders/vt.glsl");
    const char* vtSrc = vtSrcStr.c_str();
    GLuint vert = gl.CreateShader(GL_VERTEX_SHADER);
    gl.ShaderSource(vert, 1, &vtSrc, nullptr);
    gl.CompileShader(vert);
    GLint success;
    gl.GetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if(!success) {
        char log[512];
        gl.GetShaderInfoLog(vert, 512, nullptr, log);
        std::cout << log << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::string fgSrcStr = getFile("shaders/fg.glsl");
    const char* fgSrc = fgSrcStr.c_str();
    GLuint frag = gl.CreateShader(GL_FRAGMENT_SHADER);
    gl.ShaderSource(frag, 1, &fgSrc, nullptr);
    gl.CompileShader(frag);
    gl.GetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if(!success) {
        char log[512];
        gl.GetShaderInfoLog(frag, 512, nullptr, log);
        std::cout << log << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    GLuint prog = gl.CreateProgram();
    gl.AttachShader(prog, vert);
    gl.AttachShader(prog, frag);
    gl.LinkProgram(prog);
    gl.GetProgramiv(prog, GL_LINK_STATUS, &success);
    if(!success) {
        char log[512];
        gl.GetProgramInfoLog(prog, 512, nullptr, log);
        std::cout << log << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    GLuint modelLoc = gl.GetUniformLocation(prog, "model");
    GLuint viewLoc = gl.GetUniformLocation(prog, "view");
    GLuint projectionLoc = gl.GetUniformLocation(prog, "projection");

    gl.DeleteShader(vert);
    gl.DeleteShader(frag);
    gl.Enable(GL_DEPTH_TEST);

    
    glm::mat4 model = glm::identity<glm::mat4>();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int refreshRate = mode->refreshRate;
    
    float walkSpeed = 3.f;
    float delta = 1.f/static_cast<float>(refreshRate);
    float timeBegin = glfwGetTime();
    float timeEnd = glfwGetTime() + delta;

    static bool f1WasDown = false;
    static bool escWasDown = false;
    
    bool mouseFree;

    gl.ClearColor(.0f, .0f, .0f, 1.f);
    gl.UseProgram(prog);
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        timeBegin = glfwGetTime();

        bool f1Down = glfwGetKey(window, GLFW_KEY_F1);        
        if(f1Down && !f1WasDown) {
            HWND console = GetConsoleWindow();

            if (console)
                SetForegroundWindow(console);

            std::cin >> path;
            path += ".msh";
            formatMesh(getFile("meshes/"+path));
            gl.BindVertexArray(VAO);
            
            gl.BindBuffer(GL_ARRAY_BUFFER, VBO);
            gl.BufferData(
                GL_ARRAY_BUFFER,
                verts.size() * sizeof(float),
                verts.data(),
                GL_DYNAMIC_DRAW
            );
            gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            gl.BufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                inds.size() * sizeof(unsigned int),
                inds.data(),
                GL_DYNAMIC_DRAW
            );
            gl.BindVertexArray(VAO);

            SetForegroundWindow((HWND)glfwGetWin32Window(window));
        }
        f1WasDown = f1Down;

        bool escDown = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if(escDown && !escWasDown) {
            if(mouseFree) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwSetCursorPosCallback(window, nullptr);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
                glfwSetCursorPosCallback(window, mouse_callback);
            }
            mouseFree = !mouseFree;
        }
        escWasDown = escDown;
        
        glm::vec3 moveDir = glm::vec3(dir.x, 0, dir.z);
        glm::vec3 right = glm::normalize(glm::cross(moveDir, glm::vec3(0.f, 1.f, 0.f)));

        if(glfwGetKey(window, GLFW_KEY_W)) {
            eyePos += walkSpeed * moveDir * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_S)) {
            eyePos -= walkSpeed * moveDir * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_D)) {
            eyePos += walkSpeed * right * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_A)) {
            eyePos -= walkSpeed * right * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_E)) {
            eyePos.y += walkSpeed / 2 * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_Q)) {
            eyePos.y -= walkSpeed / 2 * delta;
        }

        legPos.y = eyePos.y - 2;

        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            eyePos,
            eyePos + dir,
            glm::vec3(0, 1, 0)
        );

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        gl.UniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(proj));
        gl.UniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        gl.UniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        gl.DrawElements(GL_TRIANGLES, inds.size(), GL_UNSIGNED_INT, nullptr);
        
        glfwSwapBuffers(window);
        timeEnd = glfwGetTime();
        delta = timeEnd - timeBegin;
    }
    gl.DeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}