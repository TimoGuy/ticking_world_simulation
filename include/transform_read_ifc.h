#pragma once

#include "cglm/cglm.h"


namespace world_sim
{

// Public interface for reading transforms.
class Transform_read_ifc
{
public:
    virtual void read_current_transform(mat4& out_transform, float_t t) = 0;
};

}  // namespace world_sim
