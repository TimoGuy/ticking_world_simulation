#pragma once

#include <cinttypes>
#include "jolt_physics_headers.h"


// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
    // Named enum style.
    static constexpr JPH::ObjectLayer NON_MOVING   = 0;
    static constexpr JPH::ObjectLayer MOVING       = 1;
    static constexpr JPH::ObjectLayer HIT_HURT_BOX = 2;
    static constexpr JPH::ObjectLayer NUM_LAYERS   = 3;
};

/// Class that determines if two object layers can collide
class Object_layer_pair_filter_impl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer in_object1, JPH::ObjectLayer in_object2) const override
    {
        switch (in_object1)
        {
        case Layers::NON_MOVING:
            return
                in_object2 == Layers::MOVING &&  // Non moving only collides with moving.
                in_object2 != Layers::HIT_HURT_BOX;
        case Layers::MOVING:
            return
                true && // Moving collides with everything (except hit hurt box).
                in_object2 != Layers::HIT_HURT_BOX;
        case Layers::HIT_HURT_BOX:
            return false;  // Hit/hurt boxes only do scene queries, so no collision.
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};


// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace Broad_phase_layers
{
    // Named enum style.
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::BroadPhaseLayer HIT_HURT_BOX(2);
    static constexpr uint32_t NUM_LAYERS(3);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BP_layer_interface_impl final : public JPH::BroadPhaseLayerInterface
{
public:
    BP_layer_interface_impl()
    {
        // Create a mapping table from object to broad phase layer.
        m_object_to_broad_phase[Layers::NON_MOVING]   = Broad_phase_layers::NON_MOVING;
        m_object_to_broad_phase[Layers::MOVING]       = Broad_phase_layers::MOVING;
        m_object_to_broad_phase[Layers::HIT_HURT_BOX] = Broad_phase_layers::HIT_HURT_BOX;
    }

    virtual uint32_t GetNumBroadPhaseLayers() const override
    {
        return Broad_phase_layers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return m_object_to_broad_phase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::HIT_HURT_BOX: return "HIT_HURT_BOX";
        default:													JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer m_object_to_broad_phase[Layers::NUM_LAYERS];
};
