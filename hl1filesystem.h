#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "hl1bsptypes.h"

#include <filesystem>
#include <string>
#include <fstream>

class FileSystemSearchPath
{
public:
    explicit FileSystemSearchPath(
        const std::filesystem::path &root);

    explicit FileSystemSearchPath(
        const std::string &root);

    virtual bool IsInSearchPath(
        const std::string &filename);

    virtual std::string LocateFile(
        const std::string &relativeFilename);

    virtual bool LoadFile(
        const std::string &filename,
        std::vector<valve::byte> &data);

protected:
    std::filesystem::path _root;
};

class PakSearchPath :
    public FileSystemSearchPath
{
public:
    explicit PakSearchPath(
        const std::filesystem::path &root);

    explicit PakSearchPath(
        const std::string &root);

    virtual ~PakSearchPath();

    virtual std::string LocateFile(
        const std::string &relativeFilename);

    virtual bool LoadFile(
        const std::string &filename,
        std::vector<valve::byte> &data);

private:
    void OpenPakFile();
    std::ifstream _pakFile;
    valve::hl1::tPAKHeader _header;
    std::vector<valve::hl1::tPAKLump> _files;
};

class FileSystem :
    public valve::IFileSystem
{
public:
    void FindRootFromFilePath(
        const std::string &filePath);

    virtual std::string LocateFile(
        const std::string &relativeFilename) override;

    virtual bool LoadFile(
        const std::string &filename,
        std::vector<valve::byte> &data) override;

    const std::filesystem::path &Root() const;
    const std::string &Mod() const;

private:
    std::filesystem::path _root;
    std::string _mod;
    std::vector<std::unique_ptr<FileSystemSearchPath>> _searchPaths;

    void SetRootAndMod(
        const std::filesystem::path &root,
        const std::string &mod);
};

#endif // FILESYSTEM_H
