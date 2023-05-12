#pragma once
#include <glbinding/glbinding.h>
#include <glbinding/gl/gl.h>

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>

using namespace gl;

struct Character {
    unsigned int TextureID;  // opengl texture ID of the glyph
    glm::ivec2   Size;       // Size of glyph (width and height of bitmap)
    // bearing.x horizontal position relative to the origin
    // bearing.y vertical position relative to the baseline
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    // horizontal distance in 1/64th pixels from the origin to the next origin
    unsigned int Advance;    // Offset to advance to next glyph
};

class gl_textrenderer
{
public:
    gl_textrenderer(unsigned int screen_width, unsigned int screen_height);
    ~gl_textrenderer();

    void render_text();
private:
    void load_ascii_characters();
    unsigned int create_shaders(std::string& vertex_src, std::string& fragment_src);
    unsigned int m_screen_width;
    unsigned int m_screen_height;
    std::map<char, Character> m_characters;
    glm::mat4 m_projection;
    float m_x;
    float m_y;
    float m_scale;

    unsigned int m_shader_program;
};
