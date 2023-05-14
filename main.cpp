#include <GLFW/glfw3.h>
#include <glbinding/glbinding.h>
#include <glbinding/gl/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gl_gridlines/gl_gridlines.h"
#include "gl_textrenderer/gl_textrenderer.h"

using namespace gl;

const unsigned int SCREEN_WIDTH = 500;
const unsigned int SCREEN_HEIGHT = 500;

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "gl_textrenderer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glbinding::initialize(glfwGetProcAddress);

    gl_gridlines gridlines(SCREEN_WIDTH, SCREEN_HEIGHT, 13, {1.0f, 1.0f, 1.0f, 0.1f});
    gl_textrenderer textrenderer(SCREEN_WIDTH, SCREEN_HEIGHT, "assets/Ubuntu-R.ttf", 13);
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        textrenderer.render_text("hello world", 0, 0);
        gridlines.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
