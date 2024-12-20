#ifndef _HLTYPES_H_
#define _HLTYPES_H_

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace valve
{

    namespace hl1
    {
        /* PAK */
        typedef struct sPAKLump
        {
            char name[56];
            int filepos;
            int filelen;

        } tPAKLump;

        typedef struct sPAKHeader
        {
            char signature[4];
            int lumpsOffset;
            int lumpsSize;

        } tPAKHeader;

    } // namespace hl1

    typedef unsigned char byte;
    typedef byte *byteptr;

#define CHUNK (4096)

    template <class T>
    class List
    {
    public:
        List() : size(CHUNK), data(new T[CHUNK]), count(0) {}
        virtual ~List() { this->Clear(); }

        operator T *(void) const { return data; }
        const T &operator[](int i) const { return data[i]; }
        T &operator[](int i) { return data[i]; }

        int Count() const { return count; }

        void Add(T &src)
        {
            if (count >= size)
            {
                // resize
                T *n = new T[size + CHUNK];
                for (int i = 0; i < size; i++)
                    n[i] = data[i];
                delete[] data;
                data = n;
                size += CHUNK;
            }

            data[count] = src;
            count++;
        }

        void Clear()
        {
            if (this->data != nullptr)
                delete this->data;
            this->data = nullptr;
            this->size = this->count = 0;
        }

    private:
        int size;
        int count;
        T *data;
    };

    typedef struct sVertex
    {
        glm::vec3 position;
        glm::vec2 texcoords[2];
        glm::vec3 normal;
        int bone;

    } tVertex;

    typedef struct sFace
    {
        int firstVertex;
        int vertexCount;
        unsigned int lightmap;
        size_t texture;

        int flags;
        glm::vec4 plane;

    } tFace;

    class IOpenFile
    {
    public:
        virtual bool LoadBytes(
            size_t count,
            std::vector<byte> &data,
            size_t offsetFromStart) = 0;

        virtual void Close() = 0;
    };

    class IFileSystem
    {
    public:
        virtual bool FindRootFromFilePath(
            const std::string &filePath) = 0;

        virtual std::string LocateFile(
            const std::string &relativeFilename) = 0;

        virtual bool LoadFile(
            const std::string &filename,
            std::vector<byte> &data) = 0;

        virtual valve::IOpenFile *OpenFile(
            const std::string &filename) = 0;

        const std::filesystem::path &Root() const { return _root; }

        const std::string &Mod() const { return _mod; }

    private:
        std::filesystem::path _root;
        std::string _mod;
    };

    enum AssetTypes
    {
        Bsp,
        Mdl,
        Spr,
        Tex,
        Wad,
    };

    class Asset
    {
    public:
        Asset(
            IFileSystem *fs) : _fs(fs)
        {
            static long idCounter = 0;
            _id = idCounter++;
        }

        virtual ~Asset() {}

        virtual bool Load(
            const std::string &filename) = 0;

        long Id() const { return _id; }

        virtual AssetTypes AssetType() = 0;

    protected:
        IFileSystem *_fs;

    private:
        long _id;
    };

} // namespace valve

#endif // _HLTYPES_H_
