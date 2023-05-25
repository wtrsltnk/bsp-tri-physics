#ifndef HL1SPRASSET_H_
#define HL1SPRASSET_H_

#include "../hltexture.h"
#include "../hltypes.h"
#include "hl1sprtypes.h"

#include <glad/glad.h>
#include <string>
#include <vector>

namespace valve
{

    namespace hl1
    {

        class SprAsset :
            public Asset
        {
        public:
            SprAsset(
                IFileSystem *fs);

            virtual ~SprAsset();

            virtual bool Load(
                const std::string &filename);

            // These are parsed from the mapped data
            std::vector<Texture *> _textures;
            std::vector<tFace> _faces;
            std::vector<tVertex> _vertices;

        private:
            // File format header
            tSPRHeader *_header;

            std::vector<GLuint> _frames;
        };

    } // namespace hl1

} // namespace valve

#endif // HL1SPRASSET_H_
