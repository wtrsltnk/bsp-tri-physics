#ifndef HL1MDLASSET_H
#define HL1MDLASSET_H

#include "../hltexture.h"
#include "../hltypes.h"
#include "hl1mdltypes.h"

namespace valve
{

    namespace hl1
    {

        class MdlAsset :
            public Asset
        {
        public:
            typedef struct sModel
            {
                int firstFace;
                int faceCount;
                std::vector<tFace> faces;

            } tModel;

            typedef struct sBodypart
            {
                std::vector<tModel> models;

            } tBodypart;

        public:
            MdlAsset(
                IFileSystem *fs);

            virtual ~MdlAsset();

            virtual bool Load(
                const std::string &filename);

            virtual AssetTypes AssetType() { return AssetTypes::Mdl; }

            // File format headers
            tMDLHeader *_header;
            tMDLHeader *_textureHeader;
            tMDLSequenceHeader *_animationHeaders[32];

            // These are mapped from file data
            std::vector<tMDLBodyParts> _bodyPartData;
            std::vector<tMDLTexture> _textureData;
            std::vector<short> _skinRefData;
            std::vector<short> _skinFamilyData; // not sure this contains the right data and size
            std::vector<tMDLSequenceGroup> _sequenceGroupData;
            std::vector<tMDLSequenceDescription> _sequenceData;
            std::vector<tMDLBoneController> _boneControllerData;
            std::vector<tMDLBone> _boneData;

            // These are parsed from the mapped data
            std::vector<tBodypart> _bodyparts;
            std::vector<Texture *> _textures;
            std::vector<Texture *> _lightmaps;
            std::vector<tFace> _faces;
            std::vector<tVertex> _vertices;

            int SequenceCount() const;

            int BodypartCount() const;

            tMDLAnimation *GetAnimation(
                tMDLSequenceDescription *pseqdesc);

        private:
            std::vector<byte> data;

            void LoadTextures(
                std::vector<Texture *> &textures);

            void LoadBodyParts(
                std::vector<tFace> &faces,
                std::vector<tVertex> &vertices,
                std::vector<Texture *> &lightmaps);
        };

        template <typename T>
        std::vector<T> Map(
            valve::byte *data,
            int count,
            int index)
        {
            auto rawData = (T *)(data + index);

            return std::vector<T>(rawData, rawData + count);
        }

        template <typename T>
        std::vector<T> Map(
            std::vector<valve::byte> &data,
            int count,
            int index)
        {
            return Map<T>(data.data(), count, index);
        }

        template <typename T>
        std::vector<T> MapFromHeader(
            valve::hl1::tMDLHeader *header,
            int count,
            int index)
        {
            return Map<T>((valve::byte *)header, count, index);
        }

    } // namespace hl1

} // namespace valve

#endif // HL1MDLASSET_H
