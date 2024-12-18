#ifndef HL1MDLINSTANCE_H
#define HL1MDLINSTANCE_H

#include "hl1mdlasset.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace valve
{

    namespace hl1
    {

        class MdlInstance
        {
        public:
            MdlInstance();

            virtual ~MdlInstance();

            MdlAsset *Asset;

            size_t SetSequence(
                size_t iSequence,
                bool repeat);

            float SetController(
                int iController,
                float flValue);

            float SetMouth(
                float flValue);

            float SetBlending(
                int iBlender,
                float flValue);

            int SetVisibleBodygroupModel(
                size_t bodygroup,
                int model);

            size_t SetSkin(
                size_t iValue);

            float SetSpeed(
                float speed);

            float Update(
                float prevFrame,
                std::chrono::microseconds time);

            static glm::mat4 _bonetransform[MAX_MDL_BONES];

            int _visibleModels[MAX_MDL_BODYPARTS];

        private:
            size_t Sequence = 0;
            size_t Skin = 0;
            float Frame = 0.0f;
            bool Repeat = true;
            short Controller[4];
            short Blending[4];
            short Mouth = 0;
            float Speed = 1.0f;

            const glm::mat4 *BuildSkeleton();

            void CalcBoneAdj();

            void CalcBoneQuaternion(
                int frame,
                float s,
                const tMDLBone *pbone,
                tMDLAnimation *panim,
                glm::quat &q);

            void CalcBonePosition(
                int frame,
                float s,
                const tMDLBone *pbone,
                tMDLAnimation *panim,
                glm::vec3 &pos);

            void CalcRotations(
                glm::vec3 pos[],
                glm::quat q[],
                tMDLSequenceDescription *pseqdesc,
                tMDLAnimation *panim);

            void SlerpBones(
                glm::quat q1[],
                glm::vec3 pos1[],
                glm::quat q2[],
                glm::vec3 pos2[],
                float s);

            glm::quat _adj; // FIX: non persistant, make static
        };

    } // namespace hl1

} // namespace valve

#endif // HL1MDLINSTANCE_H
