#include "shader.hpp"

#include <fstream>
#include <cstring>
#include <sstream>
#include <stack>
#include <sys/stat.h>

#include "log.hpp"

namespace {
    // Get last time modified for file
    time_t getMod(const std::string& path) {
        struct stat sb;
        if (stat(path.c_str(), &sb) == -1) {
            ADD_LOG("[shader] stat failed for '%s'\n", path.c_str());
            return (time_t) - 1;
        }
        return sb.st_mtime;
    }

    std::string toString(UniformType type) {
        switch (type) {
        case UniformType::Float:
            return "float";
        case UniformType::Vec2:
            return "vec2";
        case UniformType::Vec3:
            return "vec3";
        default:
            return "toString(type) unimplemented";
        }
    }
}

#ifdef ROCKET
Shader::Shader(const std::string& name, sync_device* rocket, const std::string& vertPath,
               const std::string& fragPath, const std::string& geomPath) :
    _progID(0),
    _filePaths(3),
    _fileMods(3),
    _name(name),
    _rocket(rocket)
{
    setVendor();
    GLuint progID = loadProgram(vertPath, fragPath, geomPath);
    if (progID != 0) _progID = progID;
}
#else
Shader::Shader(const std::string& vertPath, const std::string& fragPath,
               const std::string& geomPath) :
    _progID(0),
    _filePaths(3),
    _fileMods(3)
{
    setVendor();
    GLuint progID = loadProgram(vertPath, fragPath, geomPath);
    if (progID != 0) _progID = progID;
}
#endif // ROCKET

Shader::~Shader()
{
    glDeleteProgram(_progID);
}

#ifdef ROCKET
Shader::Shader(Shader&& other) :
    _vendor(other._vendor),
    _progID(other._progID),
    _filePaths(other._filePaths),
    _fileMods(other._fileMods),
    _uniforms(other._uniforms),
    _dynamicUniforms(other._dynamicUniforms),
    _name(other._name),
    _rocket(other._rocket),
    _rocketUniforms(other._rocketUniforms)
{
    other._progID = 0;
}
#else
Shader::Shader(Shader&& other) :
    _vendor(other._vendor),
    _progID(other._progID),
    _filePaths(other._filePaths),
    _fileMods(other._fileMods),
    _uniforms(other._uniforms),
    _dynamicUniforms(other._dynamicUniforms)
{
    other._progID = 0;
}
#endif // ROCKET

#ifdef ROCKET
void Shader::bind(double syncRow)
{
    glUseProgram(_progID);
    setDynamicUniforms();
    setRocketUniforms(syncRow);
}
#else
void Shader::bind()
{
    glUseProgram(_progID);
    setDynamicUniforms();
}
#endif // ROCKET

bool Shader::reload()
{
    // Reload shaders if some was modified
    for (auto j = 0u; j < 3; ++j) {
        for (auto i = 0u; i < _filePaths[j].size(); ++i) {
            if (_fileMods[j][i] != getMod(_filePaths[j][i])) {
                GLuint progID = loadProgram(_filePaths[1].size() > 0 ? _filePaths[1][0] : "",
                                            _filePaths[0].size() > 0 ? _filePaths[0][0] : "",
                                            _filePaths[2].size() > 0 ? _filePaths[2][0] : "");
                if (progID != 0) {
                    glDeleteProgram(_progID);
                    _progID = progID;
                    return false;
                }
                return true;
            }
        }
    }
    return false;
}

std::unordered_map<std::string, Uniform>& Shader::dynamicUniforms()
{
    return _dynamicUniforms;
}

void Shader::setFloat(const std::string& name, GLfloat value)
{
    setUniform(name, {UniformType::Float, {value, 0.f, 0.f}});
}

void Shader::setVec2(const std::string& name, GLfloat x, GLfloat y)
{
    setUniform(name, {UniformType::Vec2, {x, y, 0.f}});
}

void Shader::setVendor()
{
    const char* vendor = (const char*) glGetString(GL_VENDOR);
    if (strcmp(vendor, "NVIDIA Corporation") == 0)
        _vendor = Vendor::Nvidia;
    else if (strcmp(vendor, "Intel Inc.") == 0)
        _vendor = Vendor::Intel;
    else {
        ADD_LOG("[shader] Include aware error parsing not supported for '%s'\n", vendor);
        _vendor = Vendor::NotSupported;
    }
}

GLuint Shader::loadProgram(const std::string& vertPath, const std::string& fragPath,
                           const std::string& geomPath)
{
    // Clear vectors
    for (auto& v : _filePaths) v.clear();
    for (auto& v : _fileMods) v.clear();

    // Get a program id
    GLuint progID = glCreateProgram();

    //Load and attacth shaders
    GLuint vertexShader = loadShader(vertPath, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        glDeleteProgram(progID);
        progID = 0;
        return 0;
    }
    glAttachShader(progID, vertexShader);

    GLuint geometryShader = 0;
    if (!geomPath.empty()) {
        geometryShader = loadShader(geomPath, GL_GEOMETRY_SHADER);
        if (geometryShader == 0) {
            glDeleteShader(vertexShader);
            glDeleteProgram(progID);
            progID = 0;
            return 0;
        }
        glAttachShader(progID, geometryShader);
    }

    GLuint fragmentShader = loadShader(fragPath, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);
        glDeleteProgram(progID);
        progID = 0;
        return 0;
    }
    glAttachShader(progID, fragmentShader);

    //Link program
    glLinkProgram(progID);
    GLint programSuccess = GL_FALSE;
    glGetProgramiv(progID, GL_LINK_STATUS, &programSuccess);
    if (programSuccess == GL_FALSE) {
        ADD_LOG("[shader] Error linking program %u\n", progID);
        ADD_LOG("[shader] Error code: %d", programSuccess);
        printProgramLog(_progID);
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(progID);
        progID = 0;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);

    // Query uniforms
    GLint uCount;
    glGetProgramiv(progID, GL_ACTIVE_UNIFORMS, &uCount);
    _uniforms.clear();
    for (GLuint i = 0; i < uCount; ++i) {
        char name[64];
        GLenum glType;
        GLint size;
        glGetActiveUniform(progID, i, sizeof(name), NULL, &size, &glType, name);
        UniformType type;
        switch (glType) {
        case GL_FLOAT:
            type = UniformType::Float;
            break;
        case GL_FLOAT_VEC2:
            type = UniformType::Vec2;
            break;
        case GL_FLOAT_VEC3:
            type = UniformType::Vec3;
            break;
        default:
            ADD_LOG("[shader] Unknown uniform type %u\n", glType);
            break;
        }
        _uniforms.insert({name, std::make_pair(type, glGetUniformLocation(progID, name))});
    }

    // Rebuild uniforms
    std::unordered_map<std::string, Uniform> newDynamics;
#ifdef ROCKET
    std::unordered_map<std::string, const sync_track*> newRockets;
#endif // ROCKET
    for (auto& u : _uniforms) {
        std::string name = u.first;

#ifdef ROCKET
        if (name.length() > 1 && name[0] == 'r' && isupper(name[1])) {
            // Check type
            UniformType type = u.second.first;
            if (type != UniformType::Float) {
                ADD_LOG("[shader] '%s' should be float to use it with Rocket\n", name.c_str());
                continue;
            }
            // Add existing value if present
            if (auto existing = _rocketUniforms.find(name); existing != _rocketUniforms.end()) {
                newRockets.insert(*existing);
                continue;
            }

            // Init new
            newRockets.insert({name, sync_get_track(_rocket, (_name + ":" + name).c_str())});
        }
#endif // ROCKET

        // Skip uniforms not labelled as dynamic
        if (name.length() < 2 || name[0] != 'd' || !isupper(name[1]))
            continue;

        // Add existing value if present
        if (auto existing = _dynamicUniforms.find(name); existing != _dynamicUniforms.end()) {
            newDynamics.insert(*existing);
            continue;
        }

        // Init new uniform
        UniformType type = u.second.first;
        switch (type) {
        case UniformType::Float:
        case UniformType::Vec2:
        case UniformType::Vec3:
            newDynamics.insert({u.first, {type, {0.f, 0.f, 0.f}}});
            break;
        default:
            ADD_LOG("[shader] Unimplemented dynamic uniform of type '%s'\n", toString(type).c_str());
            break;
        }
    }
    _dynamicUniforms = newDynamics;
#ifdef ROCKET
    _rocketUniforms = newRockets;
#endif // ROCKET

    ADD_LOG("[shader] Shader %u loaded\n", progID);

    return progID;
}

GLuint Shader::loadShader(const std::string& mainPath, GLenum shaderType)
{
    GLuint shaderID = 0;
    std::string shaderStr = parseFromFile(mainPath, shaderType);
    if (!shaderStr.empty()){
        shaderID = glCreateShader(shaderType);
        const GLchar* shaderSource = shaderStr.c_str();
        glShaderSource(shaderID, 1, &shaderSource, NULL);
        glCompileShader(shaderID);
        GLint shaderCompiled = GL_FALSE;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled);
        if (shaderCompiled == GL_FALSE) {
            ADD_LOG("[shader] Unable to compile shader %u\n", shaderID);
            printShaderLog(shaderID);
            shaderID = 0;
        }
    }
    return shaderID;
}

std::string Shader::parseFromFile(const std::string& filePath, GLenum shaderType)
{
    std::ifstream sourceFile(filePath.c_str());
    std::string shaderStr;
    if (sourceFile) {
        // Push filepath and timestamp to vectors
        if (shaderType == GL_FRAGMENT_SHADER) {
            _filePaths[0].emplace_back(filePath);
            _fileMods[0].emplace_back(getMod(filePath));
        } else if (shaderType == GL_VERTEX_SHADER) {
            _filePaths[1].emplace_back(filePath);
            _fileMods[1].emplace_back(getMod(filePath));
        } else {
            _filePaths[2].emplace_back(filePath);
            _fileMods[2].emplace_back(getMod(filePath));
        }

        // Get directory path for the file for possible includes
        std::string dirPath(filePath);
        dirPath.erase(dirPath.find_last_of('/') + 1);

        // Mark file start for error parsing
        shaderStr += "// File: " + filePath + '\n';

        // Parse lines
        for (std::string line; std::getline(sourceFile, line);) {
            // Handle recursive includes, expect correct syntax
            if (line.compare(0, 9, "#include ") == 0) {
                line.erase(0, 10);
                line.pop_back();
                line = parseFromFile(dirPath + line, shaderType);
                if (line.empty()) return "";
            }
            shaderStr += line + '\n';
        }

        // Mark file end for error parsing
        shaderStr += "// File: " + filePath + '\n';
    } else {
        ADD_LOG("[shader] Unable to open file '%s'\n", filePath.c_str());
    }
    return shaderStr;
}

void Shader::printProgramLog(GLuint program) const
{
    if (glIsProgram(program) == GL_TRUE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        char* errorLog = new char[maxLength];
        glGetProgramInfoLog(program, maxLength, &maxLength, errorLog);
        ADD_LOG("%s\n", errorLog);
        delete[] errorLog;
    } else {
        ADD_LOG("[shader] ID %u is not a program\n", program);
    }
}

void Shader::printShaderLog(GLuint shader) const
{
    if (glIsShader(shader) == GL_TRUE) {
        // Get errors
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char* errorLog = new char[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        if (_vendor == Vendor::NotSupported) {
            ADD_LOG("%s\n", errorLog);
            delete[] errorLog;
            return;
        }

        // Convert error string to a stream for easy line access
        std::istringstream errorStream(errorLog);

        // Get source string
        glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &maxLength);
        char* shaderStr = new char[maxLength];
        glGetShaderSource(shader, maxLength, &maxLength, shaderStr);

        // Set up vendor specific parsing
        std::string linePrefix;
        char lineNumCutoff;
        if (_vendor == Vendor::Nvidia) {
            linePrefix = "0(";
            lineNumCutoff = ')';
        } else if (_vendor == Vendor::Intel) {
            linePrefix = "ERROR: 0:";
            lineNumCutoff = ':';
        } else {
            ADD_LOG("[shader] Unimplemented vendor parsing\n");
            ADD_LOG("%s\n", errorLog);
            delete[] errorLog;
            delete[] shaderStr;
            return;
        }

        std::string lastFile;
        // Parse correct file and line numbers to errors
        for (std::string errLine; std::getline(errorStream, errLine);) {

            // Only parse if error points to a line
            if (errLine.compare(0, linePrefix.length(), linePrefix) == 0) {
                // Extract error line in parsed source
                auto lineNumEnd = errLine.find(lineNumCutoff, linePrefix.length() + 1);
                uint32_t lineNum = std::stoi(errLine.substr(linePrefix.length(), lineNumEnd - 1));

                std::stack<std::string> files;
                std::stack<uint32_t> lines;
                // Parse the source to error, track file and line in file
                std::istringstream sourceStream(shaderStr);
                for (auto i = 0u; i < lineNum; ++i) {
                    std::string srcLine;
                    std::getline(sourceStream, srcLine);
                    if (srcLine.compare(0, 9, "// File: ") == 0) {
                        srcLine.erase(0, 9);
                        // If include-block ends, pop file and it's lines from stacks
                        if (!files.empty() && srcLine.compare(files.top()) == 0) {
                            files.pop();
                            lines.pop();
                        } else {// Push new file block to stacks
                            files.push(srcLine);
                            lines.push(0);
                        }
                    } else {
                        ++lines.top();
                    }
                }

                // Print the file if it changed from last error
                if (lastFile.empty() || lastFile.compare(files.top()) != 0) {
                    ADD_LOG("In file '%s'\n", files.top().c_str());
                    lastFile = files.top();
                }

                // Insert the correct line number to error and print
                errLine.erase(linePrefix.length(), lineNumEnd - linePrefix.length());
                errLine.insert(linePrefix.length(), std::to_string(lines.top()));
            }
            ADD_LOG("%s\n", errLine.c_str());
        }
        ADD_LOG("\n");

        delete[] errorLog;
        delete[] shaderStr;
    } else {
        ADD_LOG("[shader] ID %u is not a shader\n", shader);
    }
}

GLint Shader::getUniform(const std::string& name, UniformType type) const
{
    if (_progID == 0)
        return -1;

    auto uniform = _uniforms.find(name);
    if (uniform == _uniforms.end()) {
        ADD_LOG("[shader] Uniform '%s' not found\n", name.c_str());
        return -1;
    }

    auto [actualType, location] = uniform->second;
    if (type != actualType) {
        ADD_LOG("[shader] Uniform '%s' is not of type '%s'\n", name.c_str(), toString(type).c_str());
        return -1;
    }
    return location;
}

void Shader::setUniform(const std::string& name, const Uniform& uniform)
{
    GLint location = getUniform(name, uniform.type);
    switch (uniform.type) {
    case UniformType::Float:
        glUniform1f(location, *uniform.value);
        break;
    case UniformType::Vec2:
        glUniform2fv(location, 1, uniform.value);
        break;
    case UniformType::Vec3:
        glUniform3fv(location, 1, uniform.value);
        break;
    default:
        ADD_LOG(
            "[shader] Setting uniform of type '%s' is unimplemented\n",
            toString(uniform.type).c_str()
        );
        break;
    }
}

void Shader::setDynamicUniforms()
{
    for (auto& u : _dynamicUniforms) {
        setUniform(u.first, u.second);
    }
}

#ifdef ROCKET
void Shader::setRocketUniforms(double syncRow)
{
    for (auto& u : _rocketUniforms) {
        setUniform(u.first, {UniformType::Float, {(GLfloat)sync_get_val(u.second, syncRow), 0.f, 0.f}});
    }
}
#endif // ROCKET
