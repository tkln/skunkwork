#include "shaderProgram.hpp"

#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stack>
#include <sys/stat.h>

using std::cout;
using std::endl;

// Get last time modified for file
namespace {
    time_t getMod(const std::string& path) {
        struct stat sb;
        if (stat(path.c_str(), &sb) == -1) {
            std::cout << "[shader] stat failed for " << path << std::endl;
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

ShaderProgram::ShaderProgram(const std::string& vertPath, const std::string& fragPath,
                             const std::string& geomPath) :
    _progID(0),
    _filePaths(3),
    _fileMods(3)
{
    const char* vendor = (const char*) glGetString(GL_VENDOR);
    if (strcmp(vendor, "NVIDIA Corporation") == 0)
        _vendor = Vendor::Nvidia;
    else if (strcmp(vendor, "Intel Inc.") == 0)
        _vendor = Vendor::Intel;
    else {
        cout << "[shader] Include aware error parsing not supported for '" << vendor << "'" << endl;
        _vendor = Vendor::NotSupported;
    }

    GLuint progID = loadProgram(vertPath, fragPath, geomPath);
    if (progID != 0) _progID = progID;
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(_progID);
}

void ShaderProgram::bind() const
{
    glUseProgram(_progID);
}

bool ShaderProgram::reload()
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

std::unordered_map<std::string, Uniform>& ShaderProgram::dynamicUniforms()
{
    return _dynamicUniforms;
}

void ShaderProgram::setFloat(const std::string& name, GLfloat value)
{
    GLint location = getUniform(name, UniformType::Float);
    if (location != -1)
        glUniform1f(location, value);
}

void ShaderProgram::setVec2(const std::string& name, const GLfloat value[2])
{
    GLint location = getUniform(name, UniformType::Vec2);
    if (location != -1)
        glUniform2fv(location, 1, value);
}

void ShaderProgram::setVec3(const std::string& name, const GLfloat value[3])
{
    GLint location = getUniform(name, UniformType::Vec3);
    if (location != -1)
        glUniform3fv(location, 1, value);
}

void ShaderProgram::setDynamic()
{
    for (auto& u : _dynamicUniforms) {
        switch (u.second.type) {
        case UniformType::Float:
            setFloat(u.first, u.second.value.f);
            break;
        case UniformType::Vec2:
            setVec2(u.first, u.second.value.vec2);
            break;
        case UniformType::Vec3:
            setVec3(u.first, u.second.value.vec3);
            break;
        default:
            cout << "[shader] Setting unknown dynamic uniform of type '" << toString(u.second.type) << "'" << endl;
            break;
        }
    }
}

GLuint ShaderProgram::loadProgram(const std::string& vertPath, const std::string& fragPath,
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
        cout << "[shader] Error linking program " << progID << endl;
        cout << "Error code: " << programSuccess;
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
            cout << "[shader] Unknown uniform type " << glType << endl;
            break;
        }
        _uniforms.insert({name, std::make_pair(type, glGetUniformLocation(progID, name))});
    }

    // Rebuild dynamic uniforms
    std::unordered_map<std::string, Uniform> newDynamics;
    for (auto& u : _uniforms) {
        std::string name = u.first;

        // Skip uniforms not labelled as dynamic
        if (name[0] != 'd')
            continue;

        // Add existing value if present
        if (auto existing = _dynamicUniforms.find(name); existing != _dynamicUniforms.end()) {
            newDynamics.insert(*existing);
            continue;
        }


        // Init new
        UniformType type = u.second.first;
        switch (type) {
        case UniformType::Float:
            newDynamics.insert({u.first, {type, Uniform::Data{.f = 0.f}}});
            break;
        case UniformType::Vec2:
            newDynamics.insert({u.first, {type, Uniform::Data{.vec2 = {0.f, 0.f}}}});
            break;
        case UniformType::Vec3:
            newDynamics.insert({u.first, {type, Uniform::Data{.vec3 = {0.f, 0.f, 0.f}}}});
            break;
        default:
            cout << "[shader] Unimplemented dynamic uniform of type '" << toString(type) << "'" << endl;
            break;
        }
    }
    _dynamicUniforms = newDynamics;

    cout << "[shader] Shader " << progID << " loaded" << endl;

    return progID;
}

GLuint ShaderProgram::loadShader(const std::string& mainPath, GLenum shaderType)
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
            cout << "[shader] Unable to compile shader " << shaderID << endl;
            printShaderLog(shaderID);
            shaderID = 0;
        }
    }
    return shaderID;
}

std::string ShaderProgram::parseFromFile(const std::string& filePath, GLenum shaderType)
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
        cout << "Unable to open file " << filePath << endl;
    }
    return shaderStr;
}

void ShaderProgram::printProgramLog(GLuint program) const
{
    if (glIsProgram(program) == GL_TRUE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        char* errorLog = new char[maxLength];
        glGetProgramInfoLog(program, maxLength, &maxLength, errorLog);
        for (auto i = 0; i < maxLength; ++i)
            cout << errorLog[i];
        cout << endl;
        delete[] errorLog;
    } else {
        cout << "ID " << program << " is not a program" << endl;
    }
}

void ShaderProgram::printShaderLog(GLuint shader) const
{
    if (glIsShader(shader) == GL_TRUE) {
        // Get errors
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char* errorLog = new char[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        if (_vendor == Vendor::NotSupported) {
            cout << errorLog << endl;
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
            cout << "Unimplemented vendor" << endl;
            cout << errorLog << endl;
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
                    cout << endl << "In file " << files.top() << endl;
                    lastFile = files.top();
                }

                // Insert the correct line number to error and print
                errLine.erase(linePrefix.length(), lineNumEnd - linePrefix.length());
                errLine.insert(linePrefix.length(), std::to_string(lines.top()));
            }
            cout << errLine << endl;
        }
        cout << endl;

        delete[] errorLog;
        delete[] shaderStr;
    } else {
        cout << "ID " << shader << " is not a shader" << endl;
    }
}

GLint ShaderProgram::getUniform(const std::string& name, UniformType type) const
{
    if (_progID == 0)
        return -1;

    auto uniform = _uniforms.find(name);
    if (uniform == _uniforms.end()) {
        cout << "[shader] Uniform '" << name << "' not found"  << endl;
        return -1;
    }

    auto [actualType, location] = uniform->second;
    if (type != actualType) {
        cout << "[shader] Uniform '" << name << "' is not of type " << toString(type) << endl;
        return -1;
    }
    return location;
}
