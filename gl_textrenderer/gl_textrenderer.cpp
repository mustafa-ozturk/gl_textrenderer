#include "gl_textrenderer.h"

gl_textrenderer::gl_textrenderer(unsigned int screen_width, unsigned int screen_height, std::string font_path)
    : m_screen_width(screen_width),
      m_screen_height(screen_height),
      m_font_path(font_path),
      m_projection(glm::ortho(0.0f, (float) screen_width, 0.0f, (float) screen_height))
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

    m_shader_program = create_shader_program(vertex_shader, fragment_shader);
    load_ascii_characters();
}

void gl_textrenderer::load_ascii_characters()
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

    FT_Set_Pixel_Sizes(face, 0, 50);

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

void gl_textrenderer::render_text(std::string text, float x, float y, float scale)
{
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

    int charCount = 0;
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

        m_character ch = m_characters[c];

        glUseProgram(m_shader_program);
        glUniformMatrix4fv(glGetUniformLocation(m_shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(m_projection));
        glUniform3f(glGetUniformLocation(m_shader_program, "textColor"), 200/255.0, 60/255.0, 30/255.0);
        glBindVertexArray(VAO_chars);

        float xpos;
        // don't add bearinX for first char
        if (charCount == 0)
        {
            xpos = x * scale;
        }
        else
        {
            xpos = x + ch.Bearing.x * scale;
        }
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;


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

        charCount++;
    }
}

gl_textrenderer::~gl_textrenderer()
{

}

