#pragma once
#include <GL/glew.h>
#include <string>

////////////////////////////////////////////////////////////////////////////

class Texture {
protected:
    GLuint  texID;
public:
    Texture(void):texID(0) {}
    Texture(const std::string& fileName);
    virtual ~Texture(void);

    void bind(void) {
        glBindTexture(GL_TEXTURE_2D, texID);
    }
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
