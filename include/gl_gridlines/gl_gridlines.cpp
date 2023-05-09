#include "gl_gridlines.h"

gl_gridlines::gl_gridlines(unsigned int screen_width, unsigned int screen_height, unsigned int grid_size,
                           std::array<float, 4> line_colors)
        : m_screen_width(screen_width), m_screen_height(screen_height),
          m_grid_size(grid_size), m_line_colors(line_colors)
{
    glEnable(GL_BLEND);

    const std::string vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 pos;
        layout (location = 1) in vec4 color;

        out vec4 vertexColor;

        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * vec4(pos.xyz, 1.0);
            vertexColor = color;
        }
    )";
    const std::string fragment_shader_source = R"(
        #version 330 core

        in vec4 vertexColor;

        out vec4 FragColor;

        void main()
        {
            FragColor = vertexColor;
        }
    )";
    m_shader_program = create_shader_program(vertex_shader_source, fragment_shader_source);

    create_gridline_data();
    setup_gl_objects();
    set_projection_view();
}

gl_gridlines::~gl_gridlines()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteProgram(m_shader_program);
}

void gl_gridlines::draw()
{
    glUseProgram(m_shader_program);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_vao);
    glDrawElements(GL_LINES, m_lines * 2, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

unsigned int gl_gridlines::create_shader_program(const std::string& vertex_source, const std::string& fragment_source)
{
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* c_str_vertex = vertex_source.c_str();
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
    const char* c_str_fragment = fragment_source.c_str();
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

void gl_gridlines::create_gridline_data()
{
    // vertical lines
    for (int i = m_grid_size; i < m_screen_height; i += m_grid_size)
    {
        // first point of line
        m_verticies.push_back(0); // x
        m_verticies.push_back(i); // y
        m_verticies.push_back(1); // z

        // color
        m_verticies.push_back(m_line_colors[0]); // r
        m_verticies.push_back(m_line_colors[1]); // g
        m_verticies.push_back(m_line_colors[2]); // b
        m_verticies.push_back(m_line_colors[3]); // a

        // second point of line
        m_verticies.push_back(m_screen_width); // x
        m_verticies.push_back(i); // y
        m_verticies.push_back(1); // z

        // color
        m_verticies.push_back(m_line_colors[0]); // r
        m_verticies.push_back(m_line_colors[1]); // g
        m_verticies.push_back(m_line_colors[2]); // b
        m_verticies.push_back(m_line_colors[3]); // a

        m_lines++;
    }

    // horizontal lines
    for (int i = m_grid_size; i < m_screen_width; i += m_grid_size)
    {
        // first point of line
        m_verticies.push_back(i); // x
        m_verticies.push_back(0); // y
        m_verticies.push_back(1); // z

        // color
        m_verticies.push_back(m_line_colors[0]); // r
        m_verticies.push_back(m_line_colors[1]); // g
        m_verticies.push_back(m_line_colors[2]); // b
        m_verticies.push_back(m_line_colors[3]); // a

        // second point of line
        m_verticies.push_back(i); // x
        m_verticies.push_back(m_screen_height); // y
        m_verticies.push_back(1); // z

        // color
        m_verticies.push_back(m_line_colors[0]); // r
        m_verticies.push_back(m_line_colors[1]); // g
        m_verticies.push_back(m_line_colors[2]); // b
        m_verticies.push_back(m_line_colors[3]); // a

        m_lines++;
    }

    // horizontal center line
    m_verticies.push_back(m_screen_width / 2); // x
    m_verticies.push_back(0); // y
    m_verticies.push_back(1); // z

    // color
    m_verticies.push_back(m_line_colors[0]); // r
    m_verticies.push_back(m_line_colors[1]); // g
    m_verticies.push_back(m_line_colors[2]); // b
    m_verticies.push_back(m_line_colors[0]); // a

    m_verticies.push_back(m_screen_width / 2); // x
    m_verticies.push_back(m_screen_height); // y
    m_verticies.push_back(1); // z

    // color
    m_verticies.push_back(m_line_colors[0]); // r
    m_verticies.push_back(m_line_colors[1]); // g
    m_verticies.push_back(m_line_colors[2]); // b
    m_verticies.push_back(m_line_colors[0]); // a
    m_lines++;

    // vertical center line
    m_verticies.push_back(0); // x
    m_verticies.push_back(m_screen_height / 2); // y
    m_verticies.push_back(1); // z

    // color
    m_verticies.push_back(m_line_colors[0]); // r
    m_verticies.push_back(m_line_colors[1]); // g
    m_verticies.push_back(m_line_colors[2]); // b
    m_verticies.push_back(m_line_colors[0]); // a

    m_verticies.push_back(m_screen_width); // x
    m_verticies.push_back(m_screen_height / 2); // y
    m_verticies.push_back(1); // z

    // color
    m_verticies.push_back(m_line_colors[0]); // r
    m_verticies.push_back(m_line_colors[1]); // g
    m_verticies.push_back(m_line_colors[2]); // b
    m_verticies.push_back(m_line_colors[0]); // a
    m_lines++;

    for (int i = 0; i < m_lines * 2; i++)
    {
        m_indices.push_back(i);
    }
}

void gl_gridlines::setup_gl_objects()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_verticies.size() * sizeof(float), m_verticies.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*) nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void gl_gridlines::set_projection_view()
{
    glm::mat4 projection = glm::ortho(0.0f, (float) m_screen_width, 0.0f, (float) m_screen_height);
    glUseProgram(m_shader_program);
    glUniformMatrix4fv(glGetUniformLocation(m_shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


