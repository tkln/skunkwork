#ifndef SHADERPROGRAM_HPP
#define SHADERPROGRAM_HPP

#include <GL/gl3w.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

enum class UniformType {
    Float,
    Vec2,
    Vec3
};

struct Uniform {
    union Data {
        float f;
        float vec2[2];
        float vec3[3];
    };

    UniformType type;
    Data value;
};

class ShaderProgram
{
    enum class Vendor {
        Nvidia,
        Intel,
        NotSupported
    };

public:
    ShaderProgram(const std::string& vertPath, const std::string& fragPath,
                  const std::string& geomPath = std::string());
    ~ShaderProgram();

    void bind() const;
    bool reload();
    void setFloat(const std::string& name, GLfloat value);
    void setVec2(const std::string& name, const GLfloat value[2]);
    void setVec3(const std::string& name, const GLfloat value[3]);
    void setDynamic();
    std::unordered_map<std::string, Uniform>& dynamicUniforms();

private:
    GLuint loadProgram(const std::string& vertPath, const std::string& fragPath,
                       const std::string& geomPath);
    GLuint loadShader(const std::string& mainPath, GLenum shaderType);
    std::string parseFromFile(const std::string& filePath, GLenum shaderType);
    void printProgramLog(GLuint program) const;
    void printShaderLog(GLuint shader) const;
    GLint getUniform(const std::string& name, UniformType type) const;

    Vendor _vendor;
    GLuint _progID;
    std::vector<std::vector<std::string> > _filePaths;
    std::vector<std::vector<time_t> > _fileMods;
    std::unordered_map<std::string, std::pair<UniformType, GLint>> _uniforms;
    std::unordered_map<std::string, Uniform> _dynamicUniforms;

};

#endif // SHADERPROGRAM_HPP
