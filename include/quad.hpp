#ifndef QUAD_HPP
#define QUAD_HPP

#include <GL/gl3w.h>

class Quad
{
public:
    Quad();
    ~Quad();

    Quad(const Quad& other) = delete;
    Quad(Quad&& other);
    Quad operator=(const Quad& other) = delete;

    void render() const;

private:
    GLuint _vao;
    GLuint _vbo;

};

#endif // QUAD_HPP
