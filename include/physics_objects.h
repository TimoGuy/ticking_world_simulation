#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <utility>  // std::pair
#include <vector>
#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"


namespace phys_obj
{

// Shapes.
using Shape_ref_const = JPH::RefConst<JPH::Shape>;

class Shape_ifc
{
public:
    virtual ~Shape_ifc()
    {
        // Check that the shape handle was deleted.
        assert(m_internal_shape_handle == nullptr);
    }

    virtual bool construct_shape() = 0;
    virtual bool destroy_shape() = 0;

    Shape_ref_const& get_shape()
    {
        return m_internal_shape_handle;
    }

protected:
    // @NOTE: Use this to store the pointer to the
    //   created shape.
    Shape_ref_const m_internal_shape_handle{ nullptr };
};

class Shape_box : public Shape_ifc
{
public:
    Shape_box(JPH::Vec3Arg half_extents);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    JPH::Vec3 m_half_extents;
};

class Shape_capsule : public Shape_ifc
{
public:
    Shape_capsule(float_t radius, float_t capsule_half_height);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    float_t m_radius;
    float_t m_capsule_half_height;
};

class Shape_cylinder : public Shape_ifc
{
public:
    Shape_cylinder(float_t radius, float_t half_height);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    float_t m_radius;
    float_t m_half_height;
};

class Shape_sphere : public Shape_ifc
{
public:
    Shape_sphere(float_t radius);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    float_t m_radius;
};

class Shape_tapered_capsule : public Shape_ifc
{
public:
    Shape_tapered_capsule(float_t top_radius, float_t bottom_radius, float_t half_height);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    float_t m_top_radius;
    float_t m_bottom_radius;
    float_t m_half_height;
};

class Shape_tapered_cylinder : public Shape_ifc
{
public:
    Shape_tapered_cylinder(float_t top_radius, float_t bottom_radius, float_t half_height);

    bool construct_shape() override;
    bool destroy_shape() override;

private:
    float_t m_top_radius;
    float_t m_bottom_radius;
    float_t m_half_height;
};


// Actors.
struct Shape_w_transform_offset
{
    std::unique_ptr<Shape_ifc> shape;
    JPH::Vec3 offset;
    JPH::Quat rotation;
};

class Actor_kinematic
{
public:
    Actor_kinematic(std::vector<Shape_w_transform_offset>&& shapes);

private:
    std::vector<Shape_w_transform_offset> m_shapes;
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
    Actor_character_controller(Actor_char_ctrller_type_e type_flags, Shape_cylinder&& shape);

private:
    Actor_char_ctrller_type_e m_type;
    Shape_cylinder m_shape;
};


// Trigger.
// @NOTE: These interact with character controller actors to test if there is
//   overlap with any actors and filters out unnecessary actors.
class Trigger_kinematic
{
public:
    Trigger_kinematic(Actor_char_ctrller_type_e sensing_type_flags, Shape_box&& volume);

    void set_sensing_type_flags(Actor_char_ctrller_type_e sensing_type_flags);

    inline bool get_is_triggered(
        std::vector<Actor_character_controller*>& out_overlapping_actors)
    {
        out_overlapping_actors = m_overlapping_actors;
        return m_is_triggered;
    }

private:
    Actor_char_ctrller_type_e m_sensing_type_flags;
    Shape_box m_trigger_volume;
    bool m_is_triggered;
    std::vector<Actor_character_controller*> m_overlapping_actors;
};


// Hitboxes.
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
