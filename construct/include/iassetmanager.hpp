#ifndef IASSETMANAGER_H
#define IASSETMANAGER_H

#include <glm/glm.hpp>
#include <string>
#include <valve/hl1filesystem.h>

class IAssetManager
{
public:
    virtual ~IAssetManager() {}

    virtual valve::Asset *LoadAsset(
        const std::string &name) = 0;

    template <typename T>
    T *LoadAsset(
        const std::string &name)
    {
        return reinterpret_cast<T *>(LoadAsset(name));
    }

    virtual valve::Asset *GetAsset(
        long id) = 0;

    template <typename T>
    T *GetAsset(
        long id)
    {
        return reinterpret_cast<T *>(GetAsset(id));
    }
};

#endif // IASSETMANAGER_H
