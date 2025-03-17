#include "physics_objects.h"

#include "jolt_physics_headers.h"


// Shapes.
phys_obj::Shape_box::Shape_box(JPH::Vec3Arg half_extents)
    : m_half_extents(half_extents)
{
}

bool phys_obj::Shape_box::construct_shape()
{
    JPH::BoxShapeSettings box_settings{ m_half_extents };
    m_internal_shape_handle = box_settings.Create().Get();
}

bool phys_obj::Shape_box::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}

phys_obj::Shape_capsule::Shape_capsule(float_t radius, float_t capsule_half_height)
    : m_radius(radius)
    , m_capsule_half_height(capsule_half_height)
{
}

bool phys_obj::Shape_capsule::construct_shape()
{
    JPH::CapsuleShapeSettings capsule_settings{
        m_capsule_half_height,
        m_radius };
    m_internal_shape_handle = capsule_settings.Create().Get();
}

bool phys_obj::Shape_capsule::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}

phys_obj::Shape_cylinder::Shape_cylinder(float_t radius, float_t half_height)
    : m_radius(radius)
    , m_half_height(half_height)
{
}

bool phys_obj::Shape_cylinder::construct_shape()
{
    JPH::CylinderShapeSettings cylinder_settings{
        m_half_height,
        m_radius };
    m_internal_shape_handle = cylinder_settings.Create().Get();
}

bool phys_obj::Shape_cylinder::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}

phys_obj::Shape_sphere::Shape_sphere(float_t radius)
    : m_radius(radius)
{
}

bool phys_obj::Shape_sphere::construct_shape()
{
    JPH::SphereShapeSettings sphere_settings{ m_radius };
    m_internal_shape_handle = sphere_settings.Create().Get();
}

bool phys_obj::Shape_sphere::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}

phys_obj::Shape_tapered_capsule::Shape_tapered_capsule(float_t top_radius,
                                                       float_t bottom_radius,
                                                       float_t half_height)
    : m_top_radius(top_radius)
    , m_bottom_radius(bottom_radius)
    , m_half_height(half_height)
{
}

bool phys_obj::Shape_tapered_capsule::construct_shape()
{
    JPH::TaperedCapsuleShapeSettings capsule_settings{
        m_half_height,
        m_top_radius,
        m_bottom_radius };
    m_internal_shape_handle = capsule_settings.Create().Get();
}

bool phys_obj::Shape_tapered_capsule::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}

phys_obj::Shape_tapered_cylinder::Shape_tapered_cylinder(float_t top_radius,
                                                         float_t bottom_radius,
                                                         float_t half_height)
    : m_top_radius(top_radius)
    , m_bottom_radius(bottom_radius)
    , m_half_height(half_height)
{
}

bool phys_obj::Shape_tapered_cylinder::construct_shape()
{
    JPH::TaperedCylinderShapeSettings cylinder_settings{
        m_half_height,
        m_top_radius,
        m_bottom_radius };
    m_internal_shape_handle = cylinder_settings.Create().Get();
}

bool phys_obj::Shape_tapered_cylinder::destroy_shape()
{
    m_internal_shape_handle = nullptr;
}


