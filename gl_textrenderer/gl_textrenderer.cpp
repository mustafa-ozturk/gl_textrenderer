#include "gl_textrenderer.h"

//    centering code not sure how it will be part of API yet
//    int textWidth = 0;
//    int textHeight = 0;
//    int count = 0;
//    for (char c : text)
//    {
//        Character ch = Characters[c];
//        // pick the biggest height in the text
//        if (ch.Size.y > textHeight)
//        {
//            textHeight = ch.Size.y;
//        }
//        textWidth += ch.Advance >> 6;
//        count++;
//    }

gl_textrenderer::gl_textrenderer(unsigned int screen_width, unsigned int screen_height, std::string font_path, int pixel_height)
        : m_font_path(font_path),
          m_projection(glm::ortho(0.0f, (float) screen_width, 0.0f, (float) screen_height))
{
    std::string vertex_shader = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec4 texture_coordinates;

        out vec2 TexCoords;

        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * vec4(position.xy, 0.0, 1.0);
            TexCoords = texture_coordinates.xy;
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

    m_shader_program = create_shader_program(vertex_shader, fragment_shader);
    load_ascii_characters(pixel_height);
}

gl_textrenderer::~gl_textrenderer()
{
    glDeleteProgram(m_shader_program);
}


void gl_textrenderer::render_text(std::string text, float x, float y, float scale)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_shader_program);
    glUniformMatrix4fv(glGetUniformLocation(m_shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(m_projection));
    glUniform3f(glGetUniformLocation(m_shader_program, "textColor"), 200 / 255.0, 60 / 255.0, 30 / 255.0);

    int first_bearing_x = 0;
    for (char c: text)
    {
        m_character ch = m_characters[c];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        /*
         * This removes the bearingX of the first character,
         * substracts the length of the first bearingX
         * from the bearingX of all other characters,
         * Allowing for text to be rendered exactly
         * at the given x position.
         * */
        if (first_bearing_x == 0)
        {
            first_bearing_x = ch.Bearing.x;
            ch.Bearing.x = 0;
        } else
        {
            ch.Bearing.x -= first_bearing_x;
        }

        // xpos is given x + the characters bearingX
        float xpos = x + ch.Bearing.x * scale;
        // ypos is given y - (character height - bearingY),
        // this slightly pushes characters like 'p' under the given y (which we treat as the baseline)
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float width = ch.Size.x * scale;
        float height = ch.Size.y * scale;

        /*
         * C      D ---- ypos + height,
         *
         * A      B
         *        |
         *   xpos + width
         * FREETYPE GLYPHS ARE REVERSED: 0,0  = top left
         * */
        std::vector<float> verticies = {
                xpos,           ypos,           0.0f, 1.0f, // A
                xpos + width,   ypos,           1.0f, 1.0f, // B
                xpos,           ypos + height,  0.0f, 0.0f, // C

                xpos + width,   ypos,           1.0f, 1.0f, // B
                xpos,           ypos + height,  0.0f, 0.0f, // C
                xpos + width,   ypos + height,  1.0f, 0.0f  // D
        };

        // FIXME: very inefficient
        unsigned int VAO_chars;
        glGenVertexArrays(1, &VAO_chars);
        glBindVertexArray(VAO_chars);
        unsigned int VBO_chars;
        glGenBuffers(1, &VBO_chars);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_chars);
        glBufferData(GL_ARRAY_BUFFER, verticies.size() * sizeof(float), verticies.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glUseProgram(0);
}

void gl_textrenderer::load_ascii_characters(int pixel_height)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    // face == font
    FT_Face face;
    if (FT_New_Face(ft, m_font_path.c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, pixel_height);

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
        m_character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
        };
        m_characters.insert(std::pair<char, m_character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

unsigned int gl_textrenderer::create_shader_program(std::string& vertex_src, std::string& fragment_src)
{
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* c_str_vertex = vertex_src.c_str();
    glShaderSource(vertexShader, 1, &c_str_vertex, nullptr);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* c_str_fragment = fragment_src.c_str();
    glShaderSource(fragmentShader, 1, &c_str_fragment, nullptr);
    glCompileShader(fragmentShader);
    // check for shader compile errors
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
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


