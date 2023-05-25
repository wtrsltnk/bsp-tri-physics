#include "assetmanager.h"

#include "valve/bsp/hl1bspasset.h"
#include "valve/mdl/hl1mdlasset.h"
#include "valve/spr/hl1sprasset.h"

inline bool ends_with(
    std::string const &value,
    std::string const &ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

valve::Asset *AssetManager::LoadAsset(
    const std::string &assetName)
{
    auto found = _loadedAssets.find(assetName);

    if (found != _loadedAssets.end())
    {
        return found->second.get();
    }

    valve::Asset *asset = nullptr;

    if (ends_with(assetName, ".bsp"))
    {
        asset = new valve::hl1::BspAsset(&_fs);
    }

    if (ends_with(assetName, ".mdl"))
    {
        asset = new valve::hl1::MdlAsset(&_fs);
    }

    if (ends_with(assetName, ".spr"))
    {
        asset = new valve::hl1::SprAsset(&_fs);
    }

    if (asset != nullptr && asset->Load(assetName))
    {
        _loadedAssets.insert(std::make_pair(assetName, asset));

        return asset;
    }

    return nullptr;
}

valve::Asset *AssetManager::GetAsset(
    long id)
{
    for (auto &asset : _loadedAssets)
    {
        if (asset.second->Id() == id)
        {
            return asset.second.get();
        }
    }

    return nullptr;
}
