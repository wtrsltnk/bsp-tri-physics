#ifndef _HL1WADASSET_H_
#define _HL1WADASSET_H_

#include "../hltypes.h"
#include "hl1bsptypes.h"

#include <fstream>
#include <string>
#include <vector>

namespace valve
{

    namespace hl1
    {

        class WadAsset :
            public Asset
        {
        public:
            WadAsset(
                IFileSystem *fs);

            virtual ~WadAsset();

            bool Load(
                const std::string &filename);

            virtual AssetTypes AssetType() { return AssetTypes::Wad; }

            bool IsLoaded() const;

            int IndexOf(
                const std::string &name) const;

            byteptr LumpData(
                int index);

            static std::string FindWad(
                const std::string &wad,
                const std::vector<std::string> &hints);

            static std::vector<WadAsset *> LoadWads(
                const std::string &wads,
                IFileSystem *fs);

            static void UnloadWads(
                std::vector<WadAsset *> &wads);

        private:
            valve::IOpenFile *_file = nullptr;
            tWADHeader _header;
            tWADLump *_lumps = nullptr;
            std::vector<std::vector<byte>> _loadedLumps;
        };

    } // namespace hl1

} // namespace valve

#endif // _HL1WADASSET_H_
