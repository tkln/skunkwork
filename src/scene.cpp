#include "scene.hpp"

Scene::Scene(const std::vector<std::string>& shaders, sync_device* rocket) :
    _shaderProg(shaders[0], shaders[1], shaders.size() > 2 ? shaders[2] : "")
{
    // TODO: Parse these based on some set prefix
    /*
    for (const auto& u : syncUniforms) {
        std::string cleanUniform = u;
        int labelEnd = cleanUniform.find_last_of(':');
        if (labelEnd != std::string::npos)
            cleanUniform.erase(0, labelEnd + 1);
        _uniforms.emplace_back(cleanUniform);
        _uLocations.emplace_back(_shaderProg.getULoc(cleanUniform));
        _syncTracks.emplace_back(sync_get_track(rocket, u.c_str()));
    }
    */
}

ShaderProgram& Scene::shader()
{
    return _shaderProg;
}

void Scene::bind(double syncRow)
{
    _shaderProg.bind();
    // TODO: See constructor
    /*
    for (auto i = 0u; i < _uLocations.size(); ++i)
        glUniform1f(_uLocations[i], (float)sync_get_val(_syncTracks[i], syncRow));
    */
}

void Scene::reload()
{
    if (_shaderProg.reload()) {
        // TODO: See constructor
        /*
        _uLocations.clear();
        for (const auto& u : _uniforms)
            _uLocations.emplace_back(_shaderProg.getULoc(u));
        */
       ;
   }
}
