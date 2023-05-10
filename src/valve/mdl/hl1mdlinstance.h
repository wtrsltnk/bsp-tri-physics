#ifndef HL1MDLINSTANCE_H
#define HL1MDLINSTANCE_H

#include "hl1mdlasset.h"

namespace valve
{

    namespace hl1
    {

        class MdlInstance
        {
            MdlInstance(
                MdlAsset *asset);

            virtual ~MdlInstance();

        private:
            MdlAsset *_asset;
        };

    } // namespace hl1

} // namespace valve

#endif // HL1MDLINSTANCE_H
