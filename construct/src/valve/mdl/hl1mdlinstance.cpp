#include <valve/mdl/hl1mdlinstance.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace valve::hl1;

MdlInstance::MdlInstance() = default;

MdlInstance::~MdlInstance() = default;

glm::mat4 MdlInstance::_bonetransform[MAX_MDL_BONES] = {};

float MdlInstance::Update(
    float prevFrame,
    std::chrono::microseconds time)
{
    Frame = prevFrame;

    if (Asset == nullptr)
    {
        return Frame;
    }

    tMDLSequenceDescription &pseqdesc = Asset->_sequenceData[Sequence];

    auto dt = float(double(time.count()) / 1000000.0);
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }

    if (Frame + (dt * pseqdesc.fps * Speed) < (pseqdesc.numframes - 1) || Repeat)
    {
        Frame += dt * pseqdesc.fps * Speed;

        if (pseqdesc.numframes <= 1)
        {
            Frame = 0;
        }
        else // wrap
        {
            Frame -= (int)(Frame / (pseqdesc.numframes - 1)) * (pseqdesc.numframes - 1);
        }
    }

    BuildSkeleton();

    return Frame;
}

const glm::mat4 *MdlInstance::BuildSkeleton()
{
    static glm::vec3 pos[MAX_MDL_BONES];
    static glm::quat q[MAX_MDL_BONES];

    static glm::vec3 pos2[MAX_MDL_BONES];
    static glm::quat q2[MAX_MDL_BONES];
    static glm::vec3 pos3[MAX_MDL_BONES];
    static glm::quat q3[MAX_MDL_BONES];
    static glm::vec3 pos4[MAX_MDL_BONES];
    static glm::quat q4[MAX_MDL_BONES];

    for (int i = 0; i < MAX_MDL_BONES; i++)
    {
        _bonetransform[i] = glm::mat4(1.0f);
    }

    if (Asset == nullptr)
    {
        return _bonetransform;
    }

    if (Sequence >= Asset->_sequenceData.size())
    {
        Sequence = 0;
    }

    tMDLSequenceDescription *pseqdesc = &Asset->_sequenceData[Sequence];

    tMDLAnimation *panim = Asset->GetAnimation(pseqdesc);
    this->CalcRotations(pos, q, pseqdesc, panim);

    if (pseqdesc->numblends > 1)
    {
        panim += Asset->_boneData.size();
        this->CalcRotations(pos2, q2, pseqdesc, panim);
        float s = Blending[0] / 255.0f;

        this->SlerpBones(q, pos, q2, pos2, s);

        if (pseqdesc->numblends == 4)
        {
            panim += Asset->_boneData.size();
            this->CalcRotations(pos3, q3, pseqdesc, panim);

            panim += Asset->_boneData.size();
            this->CalcRotations(pos4, q4, pseqdesc, panim);

            s = Blending[0] / 255.0f;
            this->SlerpBones(q3, pos3, q4, pos4, s);

            s = Blending[1] / 255.0f;
            this->SlerpBones(q, pos, q3, pos3, s);
        }
    }

    for (size_t i = 0; i < Asset->_boneData.size(); i++)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos[i]) * glm::toMat4(q[i]);

        if (Asset->_boneData[i].parent == -1)
            _bonetransform[i] = m;
        else
            _bonetransform[i] = _bonetransform[Asset->_boneData[i].parent] * m;
    }

    return _bonetransform;
}

void MdlInstance::CalcBoneAdj()
{
    if (Asset == nullptr)
    {
        return;
    }

    float value;
    const std::vector<tMDLBoneController> &pbonecontroller = Asset->_boneControllerData;

    for (size_t i = 0; i < pbonecontroller.size(); i++)
    {
        if (pbonecontroller[i].index <= 3)
        {
            // check for 360% wrapping
            if (pbonecontroller[i].type & HL1_MDL_RLOOP)
                value = Controller[pbonecontroller[i].index] * (360.0f / 256.0f) + pbonecontroller[i].start;
            else
            {
                value = Controller[pbonecontroller[i].index] / 255.0f;
                if (value < 0) value = 0;
                if (value > 1.0f) value = 1.0f;
                value = (1.0f - value) * pbonecontroller[i].start + value * pbonecontroller[i].end;
            }
        }
        else
        {
            value = float(Mouth / 64.0f);
            if (value > 1.0f) value = 1.0f;
            value = (1.0f - value) * pbonecontroller[i].start + value * pbonecontroller[i].end;
        }

        switch (pbonecontroller[i].type & HL1_MDL_TYPES)
        {
            case HL1_MDL_XR:
            case HL1_MDL_YR:
            case HL1_MDL_ZR:
                this->_adj[i] = value * (glm::pi<float>() / 180.0f);
                break;
            case HL1_MDL_X:
            case HL1_MDL_Y:
            case HL1_MDL_Z:
                this->_adj[i] = value;
                break;
        }
    }
}

void MdlInstance::CalcBoneQuaternion(
    int frame,
    float s,
    const tMDLBone *pbone,
    tMDLAnimation *panim,
    glm::quat &q)
{
    int j, k;
    glm::vec3 angle1, angle2;
    tMDLAnimationValue *panimvalue;

    for (j = 0; j < 3; j++)
    {
        if (panim->offset[j + 3] == 0)
        {
            angle2[j] = angle1[j] = pbone->value[j + 3]; // default;
        }
        else
        {
            panimvalue = (tMDLAnimationValue *)((byte *)panim + panim->offset[j + 3]);
            k = frame;
            while (panimvalue->num.total <= k)
            {
                k -= panimvalue->num.total;
                panimvalue += panimvalue->num.valid + 1;
            }
            // Bah, missing blend!
            if (panimvalue->num.valid > k)
            {
                angle1[j] = panimvalue[k + 1].value;

                if (panimvalue->num.valid > k + 1)
                {
                    angle2[j] = panimvalue[k + 2].value;
                }
                else
                {
                    if (panimvalue->num.total > k + 1)
                        angle2[j] = angle1[j];
                    else
                        angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
                }
            }
            else
            {
                angle1[j] = panimvalue[panimvalue->num.valid].value;
                if (panimvalue->num.total > k + 1)
                {
                    angle2[j] = angle1[j];
                }
                else
                {
                    angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
                }
            }
            angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
            angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
        }

        if (pbone->bonecontroller[j + 3] != -1)
        {
            angle1[j] += this->_adj[pbone->bonecontroller[j + 3]];
            angle2[j] += this->_adj[pbone->bonecontroller[j + 3]];
        }
    }

    if (angle1 != angle2)
    {
        q = glm::slerp(glm::quat(angle1), glm::quat(angle2), s);
    }
    else
    {
        q = glm::quat(angle1);
    }
}

void MdlInstance::CalcBonePosition(
    int frame,
    float s,
    const tMDLBone *pbone,
    tMDLAnimation *panim,
    glm::vec3 &pos)
{
    int j, k;
    tMDLAnimationValue *panimvalue;

    for (j = 0; j < 3; j++)
    {
        pos[j] = pbone->value[j]; // default;
        if (panim->offset[j] != 0)
        {
            panimvalue = (tMDLAnimationValue *)((byte *)panim + panim->offset[j]);

            k = frame;
            // find span of values that includes the frame we want
            while (panimvalue->num.total <= k)
            {
                k -= panimvalue->num.total;
                panimvalue += panimvalue->num.valid + 1;
            }
            // if we're inside the span
            if (panimvalue->num.valid > k)
            {
                // and there's more data in the span
                if (panimvalue->num.valid > k + 1)
                    pos[j] += (panimvalue[k + 1].value * (1.0f - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
                else
                    pos[j] += panimvalue[k + 1].value * pbone->scale[j];
            }
            else
            {
                // are we at the end of the repeating values section and there's another section with data?
                if (panimvalue->num.total <= k + 1)
                    pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
                else
                    pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
            }
        }
        if (pbone->bonecontroller[j] != -1)
            pos[j] += _adj[pbone->bonecontroller[j]];
    }
}

void MdlInstance::CalcRotations(
    glm::vec3 pos[],
    glm::quat q[],
    tMDLSequenceDescription *pseqdesc,
    tMDLAnimation *panim)
{
    int frame = (int)this->Frame;
    float s = (this->Frame - frame);

    // add in programatic controllers
    this->CalcBoneAdj();

    if (Asset == nullptr)
    {
        return;
    }

    const std::vector<tMDLBone> &pbone = Asset->_boneData;
    for (size_t i = 0; i < pbone.size(); i++, panim++)
    {
        this->CalcBoneQuaternion(frame, s, &pbone[i], panim, q[i]);
        this->CalcBonePosition(frame, s, &pbone[i], panim, pos[i]);
    }

    if (pseqdesc->motiontype & HL1_MDL_X)
        pos[pseqdesc->motionbone][0] = 0.0;
    if (pseqdesc->motiontype & HL1_MDL_Y)
        pos[pseqdesc->motionbone][1] = 0.0;
    if (pseqdesc->motiontype & HL1_MDL_Z)
        pos[pseqdesc->motionbone][2] = 0.0;
}

void MdlInstance::SlerpBones(
    glm::quat q1[],
    glm::vec3 pos1[],
    glm::quat q2[],
    glm::vec3 pos2[],
    float s)
{
    if (Asset == nullptr)
    {
        return;
    }

    float s1;

    if (s < 0)
        s = 0;
    else if (s > 1.0f)
        s = 1.0f;

    s1 = 1.0f - s;

    for (size_t i = 0; i < Asset->_boneData.size(); i++)
    {
        q1[i] = glm::slerp(q1[i], q2[i], s);
        pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
        pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
        pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
    }
}

size_t MdlInstance::SetSequence(
    size_t iSequence,
    bool repeat)
{
    if (Asset == nullptr)
    {
        return 0;
    }
    if (iSequence > Asset->_sequenceData.size())
        iSequence = 0;
    if (iSequence < 0)
        iSequence = Asset->_sequenceData.size() - 1;

    Sequence = iSequence;
    Frame = 0;
    Repeat = repeat;

    return Sequence;
}

float MdlInstance::SetController(
    int iController,
    float flValue)
{
    if (Asset == nullptr)
    {
        return 0.0f;
    }

    std::vector<tMDLBoneController> &bonecontrollers = Asset->_boneControllerData;

    if (bonecontrollers.empty())
    {
        return 0.0f;
    }

    tMDLBoneController *pbonecontroller = &bonecontrollers[0];

    size_t i;

    // find first controller that matches the index
    for (i = 0; i < bonecontrollers.size(); i++, pbonecontroller++)
    {
        if (pbonecontroller->index == iController)
            break;
    }
    if (i >= bonecontrollers.size())
        return flValue;

    // wrap 0..360 if it's a rotational controller
    if (pbonecontroller->type & (HL1_MDL_XR | HL1_MDL_YR | HL1_MDL_ZR))
    {
        // ugly hack, invert value if end < start
        if (pbonecontroller->end < pbonecontroller->start)
            flValue = -flValue;

        // does the controller not wrap?
        if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
        {
            if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0f) + 180)
                flValue = flValue - 360;
            if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0f) - 180)
                flValue = flValue + 360;
        }
        else
        {
            if (flValue > 360)
                flValue = flValue - int(flValue / 360.0) * 360.0f;
            else if (flValue < 0)
                flValue = flValue + int((flValue / -360.0) + 1) * 360.0f;
        }
    }

    int setting = int(255 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start));

    if (setting < 0) setting = 0;
    if (setting > 255) setting = 255;
    Controller[iController] = setting;

    return setting * (1.0f / 255.0f) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}

float MdlInstance::SetMouth(
    float flValue)
{
    if (Asset == nullptr)
    {
        return 0.0f;
    }

    std::vector<tMDLBoneController> &bonecontrollers = Asset->_boneControllerData;

    if (bonecontrollers.empty())
    {
        return 0.0f;
    }

    tMDLBoneController *pbonecontroller = &bonecontrollers[0];

    // find first controller that matches the mouth
    for (size_t i = 0; i < bonecontrollers.size(); i++, pbonecontroller++)
    {
        if (pbonecontroller->index == 4)
            break;
    }

    // wrap 0..360 if it's a rotational controller
    if (pbonecontroller->type & (HL1_MDL_XR | HL1_MDL_YR | HL1_MDL_ZR))
    {
        // ugly hack, invert value if end < start
        if (pbonecontroller->end < pbonecontroller->start)
            flValue = -flValue;

        // does the controller not wrap?
        if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
        {
            if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0f) + 180)
                flValue = flValue - 360;
            if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0f) - 180)
                flValue = flValue + 360;
        }
        else
        {
            if (flValue > 360)
                flValue = flValue - (int)(flValue / 360.0) * 360.0f;
            else if (flValue < 0)
                flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0f;
        }
    }

    int setting = int(64 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start));

    if (setting < 0) setting = 0;
    if (setting > 64) setting = 64;
    Mouth = setting;

    return setting * (1.0f / 64.0f) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}

float MdlInstance::SetBlending(
    int iBlender,
    float flValue)
{
    if (Asset == nullptr)
    {
        return 0.0f;
    }

    tMDLSequenceDescription &pseqdesc = Asset->_sequenceData[(int)Sequence];

    if (pseqdesc.blendtype[iBlender] == 0)
        return flValue;

    if (pseqdesc.blendtype[iBlender] & (HL1_MDL_XR | HL1_MDL_YR | HL1_MDL_ZR))
    {
        // ugly hack, invert value if end < start
        if (pseqdesc.blendend[iBlender] < pseqdesc.blendstart[iBlender])
            flValue = -flValue;

        // does the controller not wrap?
        if (pseqdesc.blendstart[iBlender] + 359.0f >= pseqdesc.blendend[iBlender])
        {
            if (flValue > ((pseqdesc.blendstart[iBlender] + pseqdesc.blendend[iBlender]) / 2.0f) + 180)
                flValue = flValue - 360;
            if (flValue < ((pseqdesc.blendstart[iBlender] + pseqdesc.blendend[iBlender]) / 2.0f) - 180)
                flValue = flValue + 360;
        }
    }

    int setting = int(255 * (flValue - pseqdesc.blendstart[iBlender]) / (pseqdesc.blendend[iBlender] - pseqdesc.blendstart[iBlender]));

    if (setting < 0) setting = 0;
    if (setting > 255) setting = 255;

    Blending[iBlender] = setting;

    return setting * (1.0f / 255.0f) * (pseqdesc.blendend[iBlender] - pseqdesc.blendstart[iBlender]) + pseqdesc.blendstart[iBlender];
}

int MdlInstance::SetVisibleBodygroupModel(
    size_t bodygroup,
    int model)
{
    if (Asset == nullptr)
    {
        return -1;
    }

    if (bodygroup > Asset->_bodyparts.size())
        return -1;

    tMDLBodyParts &pbodypart = Asset->_bodyPartData[bodygroup];

    if (model > pbodypart.nummodels)
        return -1;

    this->_visibleModels[bodygroup] = model;

    return this->_visibleModels[bodygroup];
}

size_t MdlInstance::SetSkin(
    size_t iValue)
{
    if (Asset == nullptr)
    {
        return 0;
    }

    if (iValue < Asset->_skinFamilyData.size())
    {
        return Skin;
    }

    Skin = iValue;

    return iValue;
}

float MdlInstance::SetSpeed(
    float speed)
{
    Speed = speed;

    return Speed;
}
