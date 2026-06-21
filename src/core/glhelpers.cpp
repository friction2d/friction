/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "glhelpers.h"
#include "exceptions.h"
#include "appsupport.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <QFile>

QString GL_PLAIN_VERT = ":/shaders/plain.vert";
GLuint GL_PLAIN_SQUARE_VBO;

QString GL_TEXTURED_VERT = ":/shaders/textured.vert";
GLuint GL_TEXTURED_SQUARE_VBO;

void iniTexturedVShaderVBO(QGL33 * const gl) {
    float vertices[] = {
    //  positions  | texture coords
         1, -1, 0,   1, 0,   // bottom right
         1,  1, 0,   1, 1,   // top right
        -1, -1, 0,   0, 0,   // bottom left
        -1,  1, 0,   0, 1    // top left
    };

    gl->glGenBuffers(1, &GL_TEXTURED_SQUARE_VBO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, GL_TEXTURED_SQUARE_VBO);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                     vertices, GL_STATIC_DRAW);

    checkGLErrors(gl, "Error initializing Textured Vertex Shader VBO.");
}

void iniTexturedVShaderVAO(QGL33 * const gl, GLuint &VAO) {
    gl->glGenVertexArrays(1, &VAO);
    if(VAO == 0) RuntimeThrow("glGenVertexArrays failed");
    gl->glBindVertexArray(VAO);

    gl->glBindBuffer(GL_ARRAY_BUFFER, GL_TEXTURED_SQUARE_VBO);

    // position attribute
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), BUFFER_OFFSET(0));
    gl->glEnableVertexAttribArray(0);
    // texture coord attribute
    gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
    gl->glEnableVertexAttribArray(1);

    checkGLErrors(gl, "Error initializing Textured Vertex Shader VAO.");
}

void iniPlainVShaderVBO(QGL33 * const gl) {
    float vertices[] = {
        // positions
         1, -1, 0,   // bottom right
         1,  1, 0,   // top right
        -1, -1, 0,   // bottom left
        -1,  1, 0    // top left
    };

    gl->glGenBuffers(1, &GL_PLAIN_SQUARE_VBO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, GL_PLAIN_SQUARE_VBO);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                     vertices, GL_STATIC_DRAW);
    checkGLErrors(gl, "Error initializing Plain Vertex Shader VBO.");
}

void iniPlainVShaderVAO(QGL33 * const gl, GLuint &VAO) {
    gl->glGenVertexArrays(1, &VAO);
    if(VAO == 0) RuntimeThrow("glGenVertexArrays failed");
    gl->glBindVertexArray(VAO);

    gl->glBindBuffer(GL_ARRAY_BUFFER, GL_PLAIN_SQUARE_VBO);

    // position attribute
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), BUFFER_OFFSET(0));
    gl->glEnableVertexAttribArray(0);
    checkGLErrors(gl, "Error initializing Plain Vertex Shader VAO.");
}

//! @brief Checks for errors after program linking and shader compilation.
void checkCompileErrors(QGL33 * const gl,
                        const GLuint& shader,
                        const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    if(type != "PROGRAM") {
        gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success) {
            gl->glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            RuntimeThrow("ERROR::SHADER_COMPILATION_ERROR of type: " +
                         type + "\n" + infoLog);
        }
    } else {
        gl->glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success) {
            gl->glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            RuntimeThrow("ERROR::PROGRAM_LINKING_ERROR of type: " +
                         type + "\n" + infoLog);
        }
    }
}


void gIniProgram(QGL33 * const gl, GLuint& program,
                 const QString& vShaderPath,
                 const QString& fShaderPath) {
    Q_INIT_RESOURCE(coreresources);
    std::string vertexCode;
    std::string fragmentCode;
    try {
        // Use QFile to enable reading from resource file
        QFile vShaderFile(vShaderPath);
        if(!vShaderFile.open(QIODevice::ReadOnly))
            RuntimeThrow("Could not open " + vShaderPath);
        const QByteArray vData = vShaderFile.readAll();
        vertexCode = vData.toStdString();
        vShaderFile.close();

        QFile fShaderFile(fShaderPath);
        if(!fShaderFile.open(QIODevice::ReadOnly))
            RuntimeThrow("Could not open " + fShaderPath);
        const QByteArray fData = fShaderFile.readAll();
        fragmentCode = fData.toStdString();
        fShaderFile.close();
    } catch(...) {
        RuntimeThrow("Could not load shader data from file.");
    }

#ifdef USE_GLES
    auto patchShaderGLES = [](std::string& code) {
        size_t pos = code.find("#version");
        if (pos != std::string::npos) {
            size_t endLine = code.find("\n", pos);
            if (endLine != std::string::npos) {
                code.replace(pos, endLine - pos, "#version 300 es\nprecision highp float;");
            }
        }
    };
    patchShaderGLES(vertexCode);
    patchShaderGLES(fragmentCode);
#endif

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    const GLuint vertexShader = gl->glCreateShader(GL_VERTEX_SHADER);
    gl->glShaderSource(vertexShader, 1, &vShaderCode, nullptr);
    gl->glCompileShader(vertexShader);
    try {
        checkCompileErrors(gl, vertexShader, "VERTEX");
    } catch(...) {
        RuntimeThrow("Error compiling vertex shader.");
    }

    const GLuint fragmentShader = gl->glCreateShader(GL_FRAGMENT_SHADER);
    gl->glShaderSource(fragmentShader, 1, &fShaderCode, nullptr);
    gl->glCompileShader(fragmentShader);
    try {
        checkCompileErrors(gl, fragmentShader, "FRAGMENT");
    } catch(...) {
        RuntimeThrow("Error compiling fragment shader.");
    }

    program = gl->glCreateProgram();
    if(program == 0) RuntimeThrow("Error while creating program");
    gl->glAttachShader(program, vertexShader);
    gl->glAttachShader(program, fragmentShader);
    gl->glLinkProgram(program);
    try {
        checkCompileErrors(gl, program, "PROGRAM");
    } catch(...) {
        RuntimeThrow("Error linking program.");
    }

    gl->glDeleteShader(vertexShader);
    gl->glDeleteShader(fragmentShader);
}

void checkGLErrors(QGL33 * const gl, const QString &msg) {
    const GLenum glError = gl->glGetError();
    if(glError == GL_NO_ERROR) return;
    RuntimeThrow(msg + (msg.isEmpty() ? "" : "\n") + "OpenGL error " + std::to_string(glError));
}
