#include "shaders.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define max(a, b) ( (a) < (b) ? (b) : (a) )
#define min(a, b) ( (a) > (b) ? (b) : (a) )

static GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path) {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    
    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open()) {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
                        VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }
    
    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
                        FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }
    
    GLint Result = GL_FALSE;
    int InfoLogLength;
    
    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);
    
    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);
    
    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);
    
    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    std::cout << &FragmentShaderErrorMessage[0] << std::endl;
    
    // Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);
    
    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    std::cout << &ProgramErrorMessage[0] << std::endl;

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

////////////////////////////////////////////////////////////////////////////

Shader::Shader(const char * vertex_file_path, const char * fragment_file_path) {
    shaderID = LoadShaders(vertex_file_path, fragment_file_path);
}

Shader::~Shader() {
    glDeleteProgram(shaderID);
}

////////////////////////////////////////////////////////////////////////////

FixedColorShader::FixedColorShader(const char * vertex_file_path, const char * fragment_file_path)
    : Shader(vertex_file_path, fragment_file_path){

    glUseProgram(shaderID);
    matrixID = glGetUniformLocation(shaderID, "MVP");
    colorID = glGetUniformLocation(shaderID, "iColor");

    // debugging code
    if (matrixID == -1 || colorID == -1) {
        printf("*** FixedColorShader not loaded correctly\n");
    }
}

////////////////////////////////////////////////////////////////////////////

TextureShader::TextureShader(const char * vertex_file_path, const char * fragment_file_path)
        : Shader(vertex_file_path, fragment_file_path){
    matrixID = glGetUniformLocation(shaderID, "MVP");
    samplerID = glGetUniformLocation(shaderID, "sampler");

    // debugging code
    if (matrixID == -1 || samplerID == -1) {
        printf("*** FixedColorShader not loaded correctly\n");
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
