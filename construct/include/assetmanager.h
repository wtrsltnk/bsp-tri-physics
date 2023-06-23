#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <iassetmanager.hpp>
#include <map>
#include <string>

class AssetManager : public IAssetManager
{
public:
    AssetManager(
        valve::IFileSystem *fileSystem);

    valve::IFileSystem *_fs;

    valve::Asset *LoadAsset(
        const std::string &name);

    valve::Asset *GetAsset(
        long id);

private:
    std::map<std::string, std::unique_ptr<valve::Asset>> _loadedAssets;
};

#endif // ASSETMANAGER_H
