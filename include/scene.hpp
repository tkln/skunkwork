#ifndef SCENE_HPP
#define SCENE_HPP

#include <sync.h>
#include <vector>

#include "shaderProgram.hpp"

class Scene
{
public:
    // Expects shaders to be vert, frag, (geom)
    Scene(const std::vector<std::string>& shaders, sync_device* rocket);
    ~Scene() {}

    void bind(double syncRow);
    void reload();
    ShaderProgram& shader();

private:
    ShaderProgram                  _shaderProg;
    std::vector<std::string>       _uniforms;
    std::vector<GLint>             _uLocations;
    std::vector<const sync_track*> _syncTracks;

};

#endif // SCENE_HPP
