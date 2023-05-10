#include <GLFW/glfw3.h>
#include <glbinding/glbinding.h>
#include <glbinding/gl/gl.h>

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>

#include "gl_gridlines/gl_gridlines.h"

using namespace gl;

const unsigned int SCREEN_WIDTH = 500;
const unsigned int SCREEN_HEIGHT = 500;

const char *charVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;

    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)";

const char *charFragmentShader = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main()
    {
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
)";

unsigned int vertexShader;
unsigned int fragmentShader;

struct Character {
    unsigned int TextureID;  // opengl texture ID of the glyph
    glm::ivec2   Size;       // Size of glyph (width and height of bitmap)
    // bearing.x horizontal position relative to the origin
    // bearing.y vertical position relative to the baseline
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    // horizontal distance in 1/64th pixels from the origin to the next origin
    unsigned int Advance;    // Offset to advance to next glyph
};
std::map<char, Character> Characters;

unsigned int create_shaders(const char* vertex_source, const char* fragment_source)
{
    // vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_source, nullptr);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment_source, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void load_ascii_characters()
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    // face == font
    FT_Face face;
    if (FT_New_Face(ft, "assets/Ubuntu-R.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 429, 415);

    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

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


    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    load_ascii_characters();

    unsigned int charShaderProgram = create_shaders(charVertexShader, charFragmentShader);

    // projection
    {
        glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH, 0.0f, (float) SCREEN_HEIGHT);
        glUseProgram(charShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(charShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    }

    gl_gridlines gridlines(SCREEN_WIDTH, SCREEN_HEIGHT, 50, {1.0f, 1.0f, 1.0f, 0.1f});

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

//        glDisable(GL_BLEND);
        std::string text = "0";
        int textWidth = 0;
        int textHeight = 0;
        for (char c : text)
        {
            Character ch = Characters[c];
            textWidth += ch.Size.x;
            // pick the biggest height in the text
            if (ch.Size.y > textHeight)
            {
                textHeight = ch.Size.y;
            }
        }

        float x = (SCREEN_WIDTH / 2) - (textWidth / 2);
        std::cout << "SCREEN_WIDTH / 2: " << (SCREEN_WIDTH / 2) << std::endl;
        std::cout << "textWidth: " << textWidth << std::endl;
        std::cout << "textHeight: " << textHeight << std::endl;


        float y = (SCREEN_HEIGHT / 2) - (textHeight / 2);
        std::cout << "SCREEN_HEIGHT / 2: " << (SCREEN_HEIGHT / 2) << std::endl;

        float scale = 1.0f;

        for (char c : text)
        {
            unsigned int VBO_chars, VAO_chars;

            glGenVertexArrays(1, &VAO_chars);
            glBindVertexArray(VAO_chars);
            glGenBuffers(1, &VBO_chars);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_chars);
            // reserve memory, later we will update the VBO's memory when rendering characters
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

            Character ch = Characters[c];

            glUseProgram(charShaderProgram);
            glUniform3f(glGetUniformLocation(charShaderProgram, "textColor"), 200/255.0, 60/255.0, 30/255.0);
            glBindVertexArray(VAO_chars);

            float xpos = x * scale;
            // don't add bearingX for first character
            if (c != *text.begin())
            {
                xpos = x + ch.Bearing.x * scale;
            }
//            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            float ypos = y * scale;


            float width = ch.Size.x * scale;
            float height = ch.Size.y * scale;
            /*
             * C      D ypos + height,
             *
             * A      B
             *   xpos + width
             * FREETYPE GLYPHS ARE REVERSED: 0,0  = top left
             * */
            // update VBO for each character
            float char_vertices[6][4] = { // x,y,tx,ty
                    // first triangle
                    {xpos,     ypos + height,     0.0f, 0.0f}, // C
                    {xpos, ypos,                  0.0f, 1.0f}, // A
                    {xpos + width, ypos,          1.0f, 1.0f}, // B

                    // second triangle
                    {xpos,     ypos + height,     0.0f, 0.0f}, // C
                    {xpos + width, ypos,          1.0f, 1.0f}, // B
                    {xpos + width, ypos + height, 1.0f, 0.0f}  // D
            };
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_chars);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(char_vertices), char_vertices);

            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            x += (ch.Advance >> 6);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
        }

        gridlines.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
