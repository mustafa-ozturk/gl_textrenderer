#include "gl_textrenderer.h"

gl_textrenderer::gl_textrenderer(unsigned int screen_width, unsigned int screen_height)
    : m_screen_width(screen_width), m_screen_height(screen_height)
{
    std::string vertex_shader = R"(
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

    std::string fragment_shader = R"(
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

    m_shader_program = create_shaders(vertex_shader, fragment_shader);
}
