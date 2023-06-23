#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "hltypes.h"

#include <filesystem>
#include <fstream>
#include <string>

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

    virtual valve::IOpenFile *OpenFile(
        const std::string &filename);

    class FileSystemSearchPathOpenFile : public valve::IOpenFile
    {
    public:
        virtual bool LoadBytes(
            size_t count,
            std::vector<valve::byte> &data,
            size_t offsetFromStart) override;

        virtual void Close() override;

    public:
        std::string FileName;
        FileSystemSearchPath *Pack = nullptr;
        std::ifstream FileHandle;
    };

protected:
    std::filesystem::path _root;
    std::map<std::string, std::unique_ptr<FileSystemSearchPathOpenFile>> _openFiles;
    friend class FileSystemSearchPathOpenFile;

    void CloseFile(
        FileSystemSearchPathOpenFile *file);
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

    virtual valve::IOpenFile *OpenFile(
        const std::string &filename);

    class PakSearchPathOpenFile : public valve::IOpenFile
    {
    public:
        virtual bool LoadBytes(
            size_t count,
            std::vector<valve::byte> &data,
            size_t offsetFromStart) override;

        virtual void Close() override;

    public:
        std::string FileName;
        PakSearchPath *Pack = nullptr;
        size_t Size = 0;
        size_t OffsetInPack = 0;
    };

private:
    void OpenPakFile();
    std::ifstream _pakFile;
    valve::hl1::tPAKHeader _header;
    std::vector<valve::hl1::tPAKLump> _files;
    std::map<std::string, std::unique_ptr<PakSearchPathOpenFile>> _openFiles;
    friend class PakSearchPathOpenFile;

    void CloseFile(
        PakSearchPathOpenFile *file);
};

class FileSystem :
    public valve::IFileSystem
{
public:
    virtual bool FindRootFromFilePath(
        const std::string &filePath);

    virtual std::string LocateFile(
        const std::string &relativeFilename) override;

    virtual bool LoadFile(
        const std::string &filename,
        std::vector<valve::byte> &data) override;

    virtual valve::IOpenFile *OpenFile(
        const std::string &filename) override;

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
