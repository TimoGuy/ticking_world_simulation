#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>  // std::pair
#include <vector>
#include "jolt_physics_headers.h"


namespace phys_obj
{

// References.
void set_references(void* physics_system, void* body_interface);

// Shapes.
enum Shape_type : uint32_t
{
    SHAPE_TYPE_BOX = 0,
    SHAPE_TYPE_SPHERE,
    SHAPE_TYPE_CAPSULE,
    SHAPE_TYPE_TAPERED_CAPSULE,
    SHAPE_TYPE_CYLINDER,
    SHAPE_TYPE_TAPERED_CYLINDER,
    NUM_SHAPE_TYPES
};

struct Shape_params_box
{
    float_t half_x;
    float_t half_y;
    float_t half_z;
};

struct Shape_params_sphere
{
    float_t radius;
};

struct Shape_params_capsule
{
    float_t radius;
    float_t half_height;
};

struct Shape_params_tapered_capsule
{
    float_t top_radius;
    float_t bottom_radius;
    float_t half_height;
};

using Shape_params_cylinder = Shape_params_capsule;
using Shape_params_tapered_cylinder = Shape_params_tapered_capsule;

using Shape_params_ptr = void*;

using Shape_const_reference = JPH::RefConst<JPH::Shape>;

template<class T>
Shape_params_ptr construct_shape_into_memory(std::vector<uint32_t>& in_out_memory, T&& shape)
{
    if (in_out_memory.size() * sizeof(uint32_t) + sizeof(shape) >
        in_out_memory.max_size() * sizeof(uint32_t))
    {
        std::cerr
            << "ERROR: Data overflow that will cause `in_out_memory` to need reallocation."
            << std::endl;
        assert(false);
        return nullptr;
    }

    if (sizeof(shape) % sizeof(uint32_t) != 0)
    {
        std::cerr
            << "ERROR: Struct does not fit equally into `uint32_t` ("
            << sizeof(uint32_t) << "B): "
            << sizeof(shape) << "B"
            << std::endl;
        assert(false);
        return nullptr;
    }

    Shape_params_ptr ret_ptr{ nullptr };
    auto ptr_writer{ reinterpret_cast<uint32_t*>(&shape) };
    for (size_t i = 0; i < (sizeof(shape) / sizeof(uint32_t)); i++)
    {
        in_out_memory.emplace_back(ptr_writer[i]);
        if (ret_ptr == nullptr)
        {
            ret_ptr = &in_out_memory.back();
        }
    }

    return ret_ptr;
}


// Actors.
struct Shape_w_transform
{
    Shape_type shape_type;
    Shape_params_ptr shape_params{ nullptr };
    JPH::Vec3 local_position{ JPH::Vec3::sZero() };
    JPH::Quat local_rotation{ JPH::Quat::sIdentity() };
};

class Actor_kinematic
{
public:
    Actor_kinematic(JPH::RVec3 position,
                    JPH::Quat rotation,
                    std::vector<Shape_w_transform>&& shape_params);

    // Delete copy constructors.
    Actor_kinematic(const Actor_kinematic&)            = delete;
    Actor_kinematic& operator=(const Actor_kinematic&) = delete;

    // Define move constructors.
    Actor_kinematic(Actor_kinematic&&)            = default;
    Actor_kinematic& operator=(Actor_kinematic&&) = default;

    ~Actor_kinematic();

    void get_position_and_rotation(JPH::RVec3& out_position,
                                   JPH::Quat& out_rotation);
    void set_transform(JPH::RVec3Arg position, JPH::QuatArg rotation);
    void move_kinematic(JPH::RVec3Arg position, JPH::QuatArg rotation);

private:
    Shape_const_reference m_shape;
    JPH::BodyID m_body_id;
};

enum Actor_char_ctrller_type_e : uint32_t
{
    ACTOR_CC_TYPE_INVALID      = (     0),
    ACTOR_CC_TYPE_PLAYER       = (1 << 0),
    ACTOR_CC_TYPE_FRIENDLY_NPC = (1 << 1),
    ACTOR_CC_TYPE_HOSTILE_NPC  = (1 << 2),
};

// @NOTE: A cylinder shape character controller with collide-and-slide functionality
//   and moving platform, stair climbing features.
class Actor_character_controller
{
public:
    Actor_character_controller(JPH::RVec3 position,
                               Actor_char_ctrller_type_e type_flags,
                               Shape_params_cylinder&& cylinder_params);

    // Delete copy constructors.
    Actor_character_controller(const Actor_character_controller&)            = delete;
    Actor_character_controller& operator=(const Actor_character_controller&) = delete;

    // Define move constructors.
    Actor_character_controller(Actor_character_controller&&)            = default;
    Actor_character_controller& operator=(Actor_character_controller&&) = default;

    ~Actor_character_controller();

    void set_position(JPH::RVec3Arg position);
    void move(JPH::Vec3Arg velocity);

private:
    Actor_char_ctrller_type_e m_type;
    Shape_const_reference m_shape;
    JPH::Ref<JPH::Character> m_character_controller;
};


// Trigger.
// @NOTE: These interact with character controller actors to test if there is
//   overlap with any actors and filters out unnecessary actors.
class Trigger_kinematic
{
public:
    Trigger_kinematic(Actor_char_ctrller_type_e sensing_type_flags, Shape_params_box&& volume_params);

    void set_sensing_type_flags(Actor_char_ctrller_type_e sensing_type_flags);

    inline bool get_is_triggered(
        std::vector<Actor_character_controller*>& out_overlapping_actors)
    {
        out_overlapping_actors = m_overlapping_actors;
        return m_is_triggered;
    }

private:
    Actor_char_ctrller_type_e m_sensing_type_flags;
    Shape_const_reference m_trigger_volume;
    bool m_is_triggered;
    std::vector<Actor_character_controller*> m_overlapping_actors;
};


// Hitboxes.
class Hurtbox_ifc;  // Forward decl.

enum Hurtbox_type_e : uint32_t
{
    HURTBOX_TYPE_METAL_BLADE = 0,
    NUM_HURTBOX_TYPES
};

using Hitbox_callback_fn = std::function<void(Hurtbox_ifc*)>;

class Hitbox_ifc
{
public:
    Hitbox_ifc(Hitbox_callback_fn&& callback);

    virtual void update_hitbox_transform() = 0;
    void report_hit(Hurtbox_ifc* hurtbox) { m_callback(hurtbox); }

private:
    Hitbox_callback_fn m_callback;
};

// @NOTE: A collection of hitboxes connected to a skeletal animation armature.
class Hitbox_skeletal_bone : public Hitbox_ifc
{
public:
    void update_hitbox_transform() override;
};

// @NOTE: A non-solid region where hurtboxes are trying to be detected.
//   Could be a switch or something.
class Hitbox_kinematic : public Hitbox_ifc
{
public:
    void update_hitbox_transform() override;
};


// Hurtboxes.
class Hurtbox_ifc
{
public:
    Hurtbox_ifc(Hurtbox_type_e type);

    virtual void update_hurtbox_transform() = 0;
    bool check_intersecting_hitboxes_and_report_hits();

private:
    Hurtbox_type_e m_type;
};

// @NOTE: Connects to a bone of the skeletal animator and calculates the delta orientations to find the way the blade traveled. Calculates a dynamic hurtbox from this.
class Hurtbox_blade : public Hurtbox_ifc
{
public:
    virtual void update_hurtbox_transform() override;

};

// @NOTE: Essentially just a hazard.
class Hurtbox_kinematic : public Hurtbox_ifc
{
public:
    virtual void update_hurtbox_transform() override;

};

}  // namespace phys_obj
