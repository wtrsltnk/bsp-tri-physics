#include "hl1mdlinstance.h"

using namespace valve::hl1;

MdlInstance::MdlInstance(
    MdlAsset *asset)
    : _asset(asset)
{}

MdlInstance::~MdlInstance() = default;
