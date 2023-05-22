#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include "valve/hl1filesystem.h"

#include <map>
#include <string>

class AssetManager
{
public:
    FileSystem _fs;

    valve::Asset *LoadAsset(
        const std::string &name);

    template <typename T>
    T *LoadAsset(
        const std::string &name)
    {
        return reinterpret_cast<T *>(LoadAsset(name));
    }

    valve::Asset *GetAsset(
        long id);

    template <typename T>
    T *GetAsset(
        long id)
    {
        return reinterpret_cast<T *>(GetAsset(id));
    }

private:
    std::map<std::string, std::unique_ptr<valve::Asset>> _loadedAssets;
};

#endif // ASSETMANAGER_H
