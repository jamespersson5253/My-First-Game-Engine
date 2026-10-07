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
#include <array>
#include <cfloat>

std::vector<float> verts;
std::vector<unsigned int> inds;
std::vector<float> lights;
std::vector<float> radius;
std::vector<float> brightness;

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
    lights.clear();
    lights.reserve(128);

    while (std::getline(ss, line)) {
        std::stringstream ls(line);

        char type;
        ls >> type;

        if (type == 'v') {
            float x, y, z, r, g, b, nx, ny, nz;
            char c;

            ls >> x >> y >> z >> c >> r >> g >> b >> c >> nx >> ny >> nz;

            verts.insert(verts.end(), {x, y, z, r, g, b, nx, ny, nz});
        }
        else if (type == 'i') {
            unsigned int a, b, c;

            ls >> a >> b >> c;

            inds.insert(inds.end(), { a, b, c });
        }
        else if (type == 'l') {
            float x, y, z, b, r;
            ls >> x >> y >> z >> r >> b;
            lights.insert(lights.end(), {x, y, z});
            radius.insert(radius.end(), r);
            brightness.insert(brightness.end(), b);
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
    if(height == 0)
        return;
    
    gl.Viewport(0, 0, width, height);
    proj = glm::perspective(
        glm::radians(90.f),
        static_cast<float>(width) / static_cast<float>(height),
        .1f, 100.f
    );
}

glm::vec3 eyePos(0, 22, 0);
glm::vec3 legPos(0, 20, 0);
glm::vec3 dir(0, 0, -1);
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

glm::vec3 closestPointOnTriangle(
    glm::vec3 p,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);

    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    glm::vec3 bp = p - b;

    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);

    if (d3 >= 0.0f && d4 <= d3)
        return b;

    float vc = d1 * d4 - d3 * d2;

    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float t = d1 / (d1 - d3);
        return a + t * ab;
    }

    glm::vec3 cp = p - c;

    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);

    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vb = d5 * d2 - d1 * d6;

    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float t = d2 / (d2 - d6);
        return a + t * ac;
    }

    float va = d3 * d6 - d5 * d4;

    if (va <= 0.0f &&
        (d4 - d3) >= 0.0f &&
        (d5 - d6) >= 0.0f)
    {
        float t = (d4 - d3) /
                  ((d4 - d3) + (d5 - d6));

        return b + t * (c - b);
    }

    float denom = 1.0f / (va + vb + vc);

    float v = vb * denom;
    float w = vc * denom;

    return a + ab * v + ac * w;
}

float findTOI(
    glm::vec3 pos,
    glm::vec3 velocity,
    float radius,
    glm::vec3 A,
    glm::vec3 B,
    glm::vec3 C,
    float dt,
    glm::vec3& collisionNormal,
    float& penetration
)
{
    const float epsilon = 0.00001f;

    penetration = 0.0f;

    glm::vec3 faceNormal =
        glm::cross(B - A, C - A);

    float faceLength = glm::length(faceNormal);

    if (faceLength <= 0.000001f)
        return -1.0f;

    faceNormal /= faceLength;

    glm::vec3 initialPoint =
        closestPointOnTriangle(pos, A, B, C);

    glm::vec3 initialOffset =
        pos - initialPoint;

    float initialDistance =
        glm::length(initialOffset);

    // The sphere is already inside the triangle's swept volume.
    // Report the penetration so the caller can push it back out.
    if (initialDistance < radius - epsilon)
    {
        if (initialDistance > 0.000001f)
        {
            collisionNormal =
                initialOffset / initialDistance;
        }
        else
        {
            collisionNormal = faceNormal;

            if (glm::dot(collisionNormal, velocity) > 0.0f)
                collisionNormal = -collisionNormal;
        }

        penetration = radius - initialDistance;
        return 0.0f;
    }

    // Exactly touching the surface is only a collision if the sphere
    // is moving into it. Otherwise we must allow tangential movement.
    if (glm::abs(initialDistance - radius) <= epsilon)
    {
        glm::vec3 normal;

        if (initialDistance > 0.000001f)
            normal = initialOffset / initialDistance;
        else
            normal = faceNormal;

        if (glm::dot(velocity, normal) < 0.0f)
        {
            collisionNormal = normal;
            return 0.0f;
        }

        return -1.0f;
    }

    float speed = glm::length(velocity);

    if (speed <= 0.000001f)
        return -1.0f;

    // Conservative advancement. The sphere center advances by a safe
    // amount based on its current distance from the triangle.
    float t = 0.0f;

    for (int iteration = 0; iteration < 32; iteration++)
    {
        glm::vec3 center =
            pos + velocity * t;

        glm::vec3 closest =
            closestPointOnTriangle(center, A, B, C);

        glm::vec3 offset =
            center - closest;

        float distance =
            glm::length(offset);

        if (distance <= radius + epsilon)
        {
            if (distance > 0.000001f)
            {
                collisionNormal = offset / distance;
            }
            else
            {
                collisionNormal = faceNormal;

                if (glm::dot(collisionNormal, velocity) > 0.0f)
                    collisionNormal = -collisionNormal;
            }

            return t;
        }

        float distanceToContact =
            distance - radius;

        float advance =
            distanceToContact / speed;

        if (advance <= 0.000001f)
            break;

        t += advance;

        if (t > dt)
            return -1.0f;
    }

    return -1.0f;
}

float findMeshTOI(
    glm::vec3 pos,
    glm::vec3 velocity,
    float radius,
    float dt,
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices,
    glm::vec3& collisionNormal,
    float& penetration
)
{
    float closestTOI = FLT_MAX;
    penetration = 0.0f;

    float searchRadius =
        radius + glm::length(velocity) * dt;

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        // Each vertex contains:
        // x y z | r g b | nx ny nz
        constexpr size_t vertexStride = 9;

        unsigned int ia = indices[i + 0] * vertexStride;
        unsigned int ib = indices[i + 1] * vertexStride;
        unsigned int ic = indices[i + 2] * vertexStride;

        glm::vec3 A(
            vertices[ia + 0],
            vertices[ia + 1],
            vertices[ia + 2]
        );

        glm::vec3 B(
            vertices[ib + 0],
            vertices[ib + 1],
            vertices[ib + 2]
        );

        glm::vec3 C(
            vertices[ic + 0],
            vertices[ic + 1],
            vertices[ic + 2]
        );

        glm::vec3 center = (A + B + C) / 3.0f;

        float triangleRadius = glm::max(
            glm::length(A - center),
            glm::max(
                glm::length(B - center),
                glm::length(C - center)
            )
        );

        float maxDistance =
            searchRadius + triangleRadius;

        float dist2 =
            glm::dot(center - pos, center - pos);

        if (dist2 > maxDistance * maxDistance)
            continue;

        glm::vec3 triangleNormal;
        float trianglePenetration = 0.0f;

        float toi = findTOI(
            pos,
            velocity,
            radius,
            A,
            B,
            C,
            dt,
            triangleNormal,
            trianglePenetration
        );

        if (toi >= 0.0f && toi < closestTOI)
        {
            closestTOI = toi;
            collisionNormal = triangleNormal;
            penetration = trianglePenetration;
        }
    }

    if (closestTOI == FLT_MAX)
        return -1.0f;

    return closestTOI;
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
        9*sizeof(float), 
        nullptr
    );
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(
        1, 
        3, 
        GL_FLOAT, 
        GL_FALSE, 
        9*sizeof(float),
        (void*)(3*sizeof(float))
    );
    gl.EnableVertexAttribArray(1);
    gl.VertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        9*sizeof(float),
        (void*)(6*sizeof(float))
    );
    gl.EnableVertexAttribArray(2);

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

    GLint modelLoc = gl.GetUniformLocation(prog, "model");
    GLint viewLoc = gl.GetUniformLocation(prog, "view");
    GLint projectionLoc = gl.GetUniformLocation(prog, "projection");
    GLint lightLoc = gl.GetUniformLocation(prog, "lights");
    GLint radiusLoc = gl.GetUniformLocation(prog, "radius");
    GLint brightnessLoc = gl.GetUniformLocation(prog, "brightness");

    gl.DeleteShader(vert);
    gl.DeleteShader(frag);
    gl.Enable(GL_DEPTH_TEST);

    
    glm::mat4 model = glm::identity<glm::mat4>();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    float feetR = 1.f;
    float walkSpeed = 3.f;
    float delta = 0.f;
    float timeEnd = glfwGetTime();
    float timeBegin;

    glm::vec3 velocity(0, 0, 0);
    float jumpHeight = 2;

    static bool f1WasDown = false;
    static bool escWasDown = false;
    static bool onGround = false;
    
    bool mouseFree = 0;

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

            if (path.size() < 4 || path.substr(path.size() - 4) != ".msh")
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
            velocity += walkSpeed * moveDir * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_S)) {
            velocity -= walkSpeed * moveDir * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_D)) {
            velocity += walkSpeed * right * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_A)) {
            velocity -= walkSpeed * right * delta;
        }
        if(glfwGetKey(window, GLFW_KEY_SPACE) && onGround) {
            velocity.y += jumpHeight;
            onGround = false;
        }
        
        velocity.x *= glm::pow(0.14, delta);
        velocity.z *= glm::pow(0.14, delta);
        velocity.y -= 1 * delta;

        float remaining = delta;

        onGround = false;
        for (int iteration = 0; iteration < 8 && remaining > 0.0f; iteration++)
        {
            glm::vec3 collisionNormal;
            float penetration = 0.0f;

            float toi = findMeshTOI(
                legPos,
                velocity,
                feetR,
                remaining,
                verts,
                inds,
                collisionNormal,
                penetration
            );

            // Nothing hit.
            if (toi < 0.0f)
            {
                legPos += velocity * remaining;
                break;
            }

            // Move exactly to the first contact.
            legPos += velocity * toi;

            // If we started inside geometry, push the player completely
            // outside the triangle before resolving velocity.
            if (penetration > 0.0f)
            {
                legPos +=
                    collisionNormal * (penetration + 0.0001f);
            }

            // Remove only the velocity component going into the surface.
            float vn = glm::dot(velocity, collisionNormal);

            if (vn < 0.0f)
                velocity -= vn * collisionNormal;

            // Continue with the remaining frame time.
            remaining -= toi;

            if (glm::dot(
                    collisionNormal,
                    glm::vec3(0, 1, 0)
                ) >= 0.2588f)
            {
                onGround = true;
            }
        }
        
        eyePos = legPos;
        eyePos.y += 2;
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
        gl.Uniform3fv(lightLoc, 128, lights.data());
        gl.Uniform1fv(brightnessLoc, 128, brightness.data());
        gl.Uniform1fv(radiusLoc, 128, radius.data());

        gl.DrawElements(GL_TRIANGLES, static_cast<GLsizei>(inds.size()), GL_UNSIGNED_INT, nullptr);
        
        glfwSwapBuffers(window);
        timeEnd = glfwGetTime();
        delta = timeEnd - timeBegin;
    }
    gl.DeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}