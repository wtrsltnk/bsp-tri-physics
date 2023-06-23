#include <valve/mdl/hl1mdlasset.h>

#include <sstream>

using namespace valve::hl1;

MdlAsset::MdlAsset(
    IFileSystem *fs)
    : Asset(fs)
{}

MdlAsset::~MdlAsset() = default;

bool MdlAsset::Load(
    const std::string &filename)
{
    auto location = _fs->LocateFile(filename);

    if (location.empty())
    {
        return false;
    }

    auto fullpath = std::filesystem::path(location) / filename;

    if (!_fs->LoadFile(fullpath.string(), data))
    {
        return false;
    }

    this->_header = (tMDLHeader *)data.data();

    std::vector<byte> textureData;

    // preload textures
    if (this->_header->numtextures == 0)
    {

        auto textureFilename = filename.substr(0, filename.size() - 4) + "T.mdl";
        auto fullTexturePath = std::filesystem::path(location) / textureFilename;

        if (_fs->LoadFile(fullTexturePath.string(), textureData))
        {
            this->_textureHeader = (tMDLHeader *)textureData.data();
        }
        else
        {
            return false;
        }
    }
    else
    {
        textureData = data;
        this->_textureHeader = this->_header;
    }

    // preload animations
    if (this->_header->numseqgroups > 1)
    {
        for (int i = 1; i < this->_header->numseqgroups; i++)
        {
            std::stringstream seqgroupname;
            seqgroupname
                << filename.substr(0, filename.size() - 4)
                << std::setw(2) << std::setfill('0') << i
                << ".mdl";

            std::vector<unsigned char> buffer;

            if (_fs->LoadFile(seqgroupname.str(), buffer))
            {
                this->_animationHeaders[i] = (tMDLSequenceHeader *)buffer.data();
            }
        }
    }

    _textureData = Map<tMDLTexture>(textureData, _textureHeader->numtextures, _textureHeader->textureindex);
    _skinRefData = Map<short>(textureData, _textureHeader->numskinref, _textureHeader->skinindex);
    _skinFamilyData = Map<short>(textureData, _textureHeader->numskinref, _textureHeader->skinindex);
    _bodyPartData = Map<tMDLBodyParts>(data, _header->numbodyparts, _header->bodypartindex);
    _sequenceGroupData = Map<tMDLSequenceGroup>(data, _header->numseqgroups, _header->seqgroupindex);
    _sequenceData = Map<tMDLSequenceDescription>(data, _header->numseq, _header->seqindex);
    _boneControllerData = Map<tMDLBoneController>(data, _header->numbonecontrollers, _header->bonecontrollerindex);
    _boneData = Map<tMDLBone>(data, _header->numbones, _header->boneindex);

    LoadTextures(_textures);
    LoadBodyParts(_faces, _vertices, _lightmaps);

    return true;
}

void MdlAsset::LoadTextures(
    std::vector<Texture *> &_textures)
{
    for (int i = 0; i < this->_textureHeader->numtextures; i++)
    {
        Texture *t = new Texture();
        tMDLTexture *ptexture = &this->_textureData[i];

        byte *data = ((byte *)this->_textureHeader) + ptexture->index;
        byte *pal = ((byte *)this->_textureHeader) + ptexture->width * ptexture->height + ptexture->index;

        std::stringstream ss;
        ss << ptexture->name << long(*(long *)ptexture);
        t->SetName(ss.str());

        // unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight;
        int outwidth, outheight;
        int row1[256], row2[256], col1[256], col2[256];
        byte *pix1, *pix2, *pix3, *pix4;
        byte *tex, *out;

        // convert texture to power of 2
        for (outwidth = 1; outwidth < ptexture->width; outwidth <<= 1)
            ;

        if (outwidth > 256)
            outwidth = 256;

        for (outheight = 1; outheight < ptexture->height; outheight <<= 1)
            ;

        if (outheight > 256)
            outheight = 256;

        tex = out = new byte[outwidth * outheight * 4];

        for (int k = 0; k < outwidth; k++)
        {
            col1[k] = int((k + 0.25f) * (float(ptexture->width) / float(outwidth)));
            col2[k] = int((k + 0.75f) * (float(ptexture->width) / float(outwidth)));
        }

        for (int k = 0; k < outheight; k++)
        {
            row1[k] = (int)((k + 0.25f) * (ptexture->height / (float)outheight)) * ptexture->width;
            row2[k] = (int)((k + 0.75f) * (ptexture->height / (float)outheight)) * ptexture->width;
        }

        // scale down and convert to 32bit RGB
        for (int k = 0; k < outheight; k++)
        {
            for (int j = 0; j < outwidth; j++, out += 4)
            {
                pix1 = &pal[data[row1[k] + col1[j]] * 3];
                pix2 = &pal[data[row1[k] + col2[j]] * 3];
                pix3 = &pal[data[row2[k] + col1[j]] * 3];
                pix4 = &pal[data[row2[k] + col2[j]] * 3];

                out[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
                out[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
                out[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
                out[3] = 0xFF;
            }
        }

        t->SetData(outwidth, outheight, 4, tex);
        delete[] tex;

        _textureData[i].index = static_cast<int>(_textures.size());
        _textures.push_back(t);
    }
}

void MdlAsset::LoadBodyParts(
    std::vector<tFace> &faces,
    std::vector<tVertex> &_vertices,
    std::vector<Texture *> &lightmaps)
{
    float s, t;
    short type;

    Texture *lm = new Texture();
    lm->SetDimentions(32, 32, 3);
    lm->Fill(glm::vec4(255, 255, 255, 255));
    lightmaps.push_back(lm);

    this->_bodyparts.resize(this->_header->numbodyparts);
    for (int i = 0; i < this->_header->numbodyparts; i++)
    {
        tMDLBodyParts &part = this->_bodyPartData[i];
        tBodypart &b = this->_bodyparts[i];
        b.models.resize(part.nummodels);

        std::vector<tMDLModel> models = MapFromHeader<tMDLModel>(_header, part.nummodels, part.modelindex);
        for (size_t j = 0; j < models.size(); j++)
        {
            tMDLModel &model = models[j];
            tModel &m = b.models[j];
            m.firstFace = static_cast<int>(faces.size());
            m.faceCount = model.nummesh;
            m.faces.resize(model.nummesh);

            auto vertices = MapFromHeader<glm::vec3>(_header, model.numverts, model.vertindex);
            auto vertexBones = MapFromHeader<byte>(_header, model.numverts, model.vertinfoindex);
            auto normals = MapFromHeader<glm::vec3>(_header, model.numnorms, model.normindex);
            auto meshes = MapFromHeader<tMDLMesh>(_header, model.nummesh, model.meshindex);

            for (int k = 0; k < model.nummesh; k++)
            {
                tMDLMesh &mesh = meshes[k];
                tFace &e = m.faces[k];

                e.firstVertex = static_cast<int>(_vertices.size());
                e.lightmap = 0;
                e.texture = this->_textureData[this->_skinRefData[mesh.skinref]].index;

                short *ptricmds = (short *)((byte *)this->_header + mesh.triindex);

                s = 1.0f / float(this->_textureData[this->_skinRefData[mesh.skinref]].width);
                t = 1.0f / float(this->_textureData[this->_skinRefData[mesh.skinref]].height);

                while ((type = *(ptricmds++)) != 0)
                {
                    tVertex first, prev;
                    for (int l = 0; l < abs(type); l++, ptricmds += 4)
                    {
                        tVertex v;

                        v.position = vertices[ptricmds[0]];
                        v.normal = normals[ptricmds[1]];
                        v.texcoords[0] = v.texcoords[1] = glm::vec2(ptricmds[2] * s, ptricmds[3] * t);
                        v.bone = int(vertexBones[ptricmds[0]]);

                        if (type < 0) // TRIANGLE_FAN
                        {
                            if (l == 0)
                                first = v;
                            else if (l == 1)
                                prev = v;
                            else
                            {
                                _vertices.push_back(first);
                                _vertices.push_back(prev);
                                _vertices.push_back(v);

                                // laatste statement
                                prev = v;
                            }
                        }
                        else // TRIANGLE_STRIP
                        {
                            if (l == 0)
                                first = v;
                            else if (l == 1)
                                prev = v;
                            else
                            {
                                if (l & 1)
                                {
                                    _vertices.push_back(first);
                                    _vertices.push_back(v);
                                    _vertices.push_back(prev);
                                }
                                else
                                {
                                    _vertices.push_back(first);
                                    _vertices.push_back(prev);
                                    _vertices.push_back(v);
                                }

                                // laatste statement
                                first = prev;
                                prev = v;
                            }
                        }
                    }
                }
                e.vertexCount = static_cast<int>(_vertices.size() - e.firstVertex);
                faces.push_back(e);
            }
        }
    }
}

int MdlAsset::SequenceCount() const
{
    return this->_header->numseq;
}

int MdlAsset::BodypartCount() const
{
    return this->_header->numbodyparts;
}

tMDLAnimation *MdlAsset::GetAnimation(
    tMDLSequenceDescription *pseqdesc)
{
    if (pseqdesc->seqgroup == 0)
    {
        tMDLSequenceGroup &pseqgroup = this->_sequenceGroupData[pseqdesc->seqgroup];

        return (tMDLAnimation *)((byte *)this->_header + pseqgroup.unused2 + pseqdesc->animindex);
    }

    return (tMDLAnimation *)((byte *)this->_animationHeaders[pseqdesc->seqgroup] + pseqdesc->animindex);
}
