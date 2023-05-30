#include "hl1sprasset.h"
#include "../hltexture.h"

using namespace valve::hl1;

SprAsset::SprAsset(
    IFileSystem *fs)
    : Asset(fs)
{}

SprAsset::~SprAsset() = default;

bool SprAsset::Load(
    const std::string &filename)
{
    auto location = _fs->LocateFile(filename);

    if (location.empty())
    {
        return false;
    }

    auto fullpath = std::filesystem::path(location) / filename;

    std::vector<byte> data;

    if (!_fs->LoadFile(fullpath.string(), data))
    {
        return false;
    }

    _header = (tSPRHeader *)data.data();

    _type = _header->type;

    int w = int(_header->width / 2.0f);
    int h = int(_header->height / 2.0f);

    tVertex verts[4];
    verts[0].position = glm::vec3(-w, 0.0f, -h);
    verts[0].texcoords[0] = glm::vec2(0.0f, 1.0f);
    verts[0].bone = -1;
    verts[1].position = glm::vec3(w, 0.0f, -h);
    verts[1].texcoords[0] = glm::vec2(1.0f, 1.0f);
    verts[1].bone = -1;
    verts[2].position = glm::vec3(w, 0.0f, h);
    verts[2].texcoords[0] = glm::vec2(1.0f, 0.0f);
    verts[2].bone = -1;
    verts[3].position = glm::vec3(-w, 0.0f, h);
    verts[3].texcoords[0] = glm::vec2(0.0f, 0.0f);
    verts[3].bone = -1;

    for (int i = 0; i < 4; i++)
    {
        _vertices.push_back(verts[i]);
    }

    short paletteColorCount = *(short *)(data.data() + sizeof(tSPRHeader));
    byte *palette = (byte *)(data.data() + sizeof(tSPRHeader) + sizeof(short));
    byte *tmp = (byte *)(palette + (paletteColorCount * 3));

    for (int f = 0; f < _header->numframes; f++)
    {
        eSpriteFrameType frames = *(eSpriteFrameType *)tmp;
        tmp += sizeof(eSpriteFrameType);
        if (frames == SPR_SINGLE)
        {
            tFace face;
            face.firstVertex = 0;
            face.vertexCount = 4;
            face.texture = _textures.size();
            face.lightmap = 0;
            _faces.push_back(face);

            tSPRFrame *frame = (tSPRFrame *)tmp;
            tmp += sizeof(tSPRFrame);
            unsigned char *textureData = new unsigned char[frame->width * frame->height * 4];
            for (int y = 0; y < frame->height; y++)
            {
                for (int x = 0; x < frame->width; x++)
                {
                    int item = x * frame->height + y;
                    unsigned char index = tmp[item];
                    textureData[item * 4] = palette[index * 3];
                    textureData[item * 4 + 1] = palette[index * 3 + 1];
                    textureData[item * 4 + 2] = palette[index * 3 + 2];
                    textureData[item * 4 + 3] = (item == 255 ? 0 : 255);
                }
            }
            auto tex = new Texture();
            tex->SetData(frame->width, frame->height, 4, textureData);
            _textures.push_back(tex);

            delete[] textureData;

            tmp += frame->width * frame->height;
        }
    }

    return true;
}
