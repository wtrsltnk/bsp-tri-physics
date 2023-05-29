#include "hl1wadasset.h"

#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>
#include <sstream>

using namespace valve::hl1;

WadAsset::WadAsset(
    IFileSystem *fs)
    : Asset(fs)
{}

WadAsset::~WadAsset()
{
    if (IsLoaded())
    {
        _file->Close();
    }
}

bool WadAsset::Load(
    const std::string &filename)
{
    _header.lumpsCount = 0;
    _header.lumpsOffset = 0;
    _header.signature[0] = '\0';

    _file = _fs->OpenFile(filename);

    if (_file == nullptr)
    {
        return false;
    }

    std::vector<valve::byte> data;

    if (!_file->LoadBytes(sizeof(tWADHeader), data, 0))
    {
        _file->Close();

        return false;
    }

    _header = *reinterpret_cast<tWADHeader *>(data.data());

    if (std::string(_header.signature, 4) != HL1_WAD_SIGNATURE)
    {
        _file->Close();

        return false;
    }

    _lumps = new tWADLump[_header.lumpsCount];

    if (!_file->LoadBytes(_header.lumpsCount * sizeof(tWADLump), data, _header.lumpsOffset))
    {
        _file->Close();

        return false;
    }

    memcpy(_lumps, data.data(), _header.lumpsCount * sizeof(tWADLump));

    _loadedLumps.resize(_header.lumpsCount);

    return true;
}

bool WadAsset::IsLoaded() const
{
    return _file != nullptr;
}

bool icasecmp(
    const std::string &l,
    const std::string &r)
{
    return l.size() == r.size() &&
           std::equal(
               l.cbegin(),
               l.cend(),
               r.cbegin(),
               [](std::string::value_type l1, std::string::value_type r1) { return std::toupper(l1) == std::toupper(r1); });
}

int WadAsset::IndexOf(
    const std::string &name) const
{
    for (int l = 0; l < _header.lumpsCount; ++l)
    {
        if (icasecmp(name, _lumps[l].name))
        {
            return l;
        }
    }

    return -1;
}

valve::byteptr WadAsset::LumpData(
    int index)
{
    if (index >= _header.lumpsCount || index < 0)
    {
        return nullptr;
    }

    if (_loadedLumps[index].empty())
    {
        if (!_file->LoadBytes(_lumps[index].size, _loadedLumps[index], _lumps[index].offset))
        {
            _file->Close();

            return nullptr;
        }
    }

    return _loadedLumps[index].data();
}

std::vector<std::string> split(
    const std::string &subject,
    const char delim = '\n')
{
    std::vector<std::string> result;

    std::istringstream f(subject);
    std::string s;

    while (getline(f, s, delim))
    {
        result.push_back(s);
    }

    return result;
}

// Answer from Stackoverflow: http://stackoverflow.com/a/9670795
template <class Stream, class Iterator>
void join(
    Stream &s,
    Iterator first,
    Iterator last,
    char const *delim = "\n")
{
    if (first >= last)
    {
        return;
    }

    s << *first++;

    for (; first != last; ++first)
    {
        s << delim << *first;
    }
}

std::vector<WadAsset *> WadAsset::LoadWads(
    const std::string &wads,
    IFileSystem *fs)
{
    std::vector<WadAsset *> result;

    std::istringstream f(wads);
    std::string s;
    while (getline(f, s, ';'))
    {
        auto wadPath = std::filesystem::path(s);
        auto location = fs->LocateFile(wadPath.filename().string());
        if (location.empty())
        {
            continue;
        }

        WadAsset *wad = new WadAsset(fs);

        auto fullWadPath = location / wadPath.filename();

        if (wad->Load(fullWadPath.string()))
        {
            result.push_back(wad);
        }
        else
        {
            spdlog::error("Unable to load wad files @ {0}", fullWadPath.string());
            delete wad;
        }
    }

    return result;
}

std::string WadAsset::FindWad(
    const std::string &wad,
    const std::vector<std::string> &hints)
{
    std::string tmp = wad;
    std::replace(tmp.begin(), tmp.end(), '/', '\\');
    std::vector<std::string> wadComponents = split(tmp, '\\');

    auto wadFile = wadComponents[wadComponents.size() - 1];
    for (auto &hint : hints)
    {
        std::filesystem::path tmp = std::filesystem::path(hint) / wadFile;
        std::ifstream f(tmp);
        if (f.good())
        {
            f.close();
            return tmp.string();
        }
    }

    auto modDir = wadComponents[wadComponents.size() - 2];

    // When the wad file is not found, we might wanna check original wad string for a possible mod directory
    std::filesystem::path lastTry = std::filesystem::path(hints[hints.size() - 1]) / modDir / wadFile;
    std::ifstream f(lastTry);
    if (f.good())
    {
        f.close();
        return lastTry.string();
    }

    return wad;
}

void WadAsset::UnloadWads(
    std::vector<WadAsset *> &wads)
{
    while (wads.empty() == false)
    {
        WadAsset *wad = wads.back();
        wads.pop_back();
        delete wad;
    }
}
