#ifndef SKUNKWORK_SHADER_HPP
#define SKUNKWORK_SHADER_HPP

#include <GL/gl3w.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#ifdef ROCKET
#include <sync.h>
#endif // ROCKET

enum class UniformType {
    Float,
    Vec2,
    Vec3
};

struct Uniform {
    UniformType type;
    float value[3];
};

class Shader
{
    enum class Vendor {
        Nvidia,
        Intel,
        NotSupported
    };

public:
#ifdef ROCKET
    Shader(const std::string& name, sync_device* rocket, const std::string& vertPath,
           const std::string& fragPath, const std::string& geomPath = "");
#else
    Shader(const std::string& vertPath, const std::string& fragPath,
           const std::string& geomPath = "");
#endif // ROCKET

    ~Shader();

    Shader(const Shader& other) = delete;
    Shader(Shader&& other);
    Shader operator=(const Shader& other);

#ifdef ROCKET
    void bind(double syncRow);
#else
    void bind();
#endif // ROCKET
    bool reload();
    void setFloat(const std::string& name, GLfloat value);
    void setVec2(const std::string& name, GLfloat x, GLfloat y);
    std::unordered_map<std::string, Uniform>& dynamicUniforms();

    GLuint _progID;

private:
    void setVendor();
    GLuint loadProgram(const std::string& vertPath, const std::string& fragPath,
                       const std::string& geomPath);
    GLuint loadShader(const std::string& mainPath, GLenum shaderType);
    std::string parseFromFile(const std::string& filePath, GLenum shaderType);
    void printProgramLog(GLuint program) const;
    void printShaderLog(GLuint shader) const;
    GLint getUniform(const std::string& name, UniformType type) const;
    void setUniform(const std::string& name, const Uniform& uniform);
    void setDynamicUniforms();
#ifdef ROCKET
    void setRocketUniforms(double syncRow);
#endif // ROCKET

    Vendor _vendor;
    std::vector<std::vector<std::string> > _filePaths;
    std::vector<std::vector<time_t> > _fileMods;
    std::unordered_map<std::string, std::pair<UniformType, GLint>> _uniforms;
    std::unordered_map<std::string, Uniform> _dynamicUniforms;
#ifdef ROCKET
    std::string _name;
    sync_device* _rocket;
    std::unordered_map<std::string, const sync_track*> _rocketUniforms;
#endif // ROCKET

};

#endif // SKUNKWORK_SHADER_HPP
