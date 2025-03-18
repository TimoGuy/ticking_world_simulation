#pragma once

#include "jolt_physics_headers.h"


/// Class that determines if an object layer can collide with a broadphase layer
class Object_vs_broad_phase_layer_filter_impl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer in_layer1, JPH::BroadPhaseLayer in_layer2) const override
    {
        switch (in_layer1)
        {
        case Layers::NON_MOVING:
            return
                in_layer2 == Broad_phase_layers::MOVING &&
                in_layer2 != Broad_phase_layers::HIT_HURT_BOX;
        case Layers::MOVING:
            return
                true &&
                in_layer2 != Broad_phase_layers::HIT_HURT_BOX;
        case Layers::HIT_HURT_BOX:
            return false;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};
