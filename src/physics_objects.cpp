#include "physics_objects.h"

#include "cglm/cglm.h"
#include "jolt_physics_headers.h"
#include "jolt_phys_impl__layers.h"


namespace phys_obj
{

static JPH::PhysicsSystem* s_physics_system{ nullptr };
static JPH::BodyInterface* s_body_interface_ptr{ nullptr };

Shape_const_reference create_shape(Shape_type shape_type,
                                   Shape_params_ptr shape_param);

}  // namespace phys_obj


// References.
void phys_obj::set_references(void* physics_system, void* body_interface)
{
    s_physics_system = reinterpret_cast<JPH::PhysicsSystem*>(physics_system);
    s_body_interface_ptr = reinterpret_cast<JPH::BodyInterface*>(body_interface);
}


// Actors.
phys_obj::Actor_kinematic::Actor_kinematic(JPH::RVec3 position,
                                           JPH::Quat rotation,
                                           std::vector<Shape_w_transform>&& shape_params)
{
    assert(!shape_params.empty());

    // Create shapes.
    std::vector<Shape_const_reference> shapes;
    shapes.reserve(shape_params.size());
    for (auto& shape_param : shape_params)
    {
        shapes.emplace_back(
            create_shape(shape_param.shape_type,
                         shape_param.shape_params));
    }

    assert(shapes.size() == shape_params.size());

    if (shapes.size() == 1)
    {
        // Single shape.
        // @NOTE: For debug just make sure that the transform is identity.
        //   FYI: Normally the 
        assert(shape_params[0].local_position.IsNearZero());
        assert(shape_params[0].local_rotation.IsClose(JPH::Quat::sIdentity()));
        m_shape = shapes[0];
    }
    else
    {
        // Compound shape.
        JPH::StaticCompoundShapeSettings compound_settings;
        for (size_t i = 0; i < shape_params.size(); i++)
        {
            auto& shape_param{ shape_params[i] };
            compound_settings.AddShape(shape_param.local_position,
                                       shape_param.local_rotation,
                                       shapes[i]);
        }
        m_shape = compound_settings.Create().Get();
    }

    // Create kinematic body.
    assert(s_body_interface_ptr != nullptr);
    m_body_id =
        s_body_interface_ptr->CreateAndAddBody(
            JPH::BodyCreationSettings(m_shape,
                                      position,
                                      rotation,
                                      JPH::EMotionType::Kinematic,
                                      Layers::MOVING),
            JPH::EActivation::Activate);
}

phys_obj::Actor_kinematic::~Actor_kinematic()
{
    // @TODO: STUB
    assert(false);
}

phys_obj::Actor_character_controller::Actor_character_controller(
    JPH::RVec3 position,
    Actor_char_ctrller_type_e type_flags,
    Shape_params_cylinder&& cylinder_params)
    : m_type(type_flags)
{
    m_shape = create_shape(Shape_type::SHAPE_TYPE_CYLINDER,
                           &cylinder_params);

    // Create character controller for collide-and-slide method.
    JPH::Ref<JPH::CharacterSettings> settings{ new JPH::CharacterSettings };
    settings->mMaxSlopeAngle = glm_rad(46.0f);
    settings->mLayer = Layers::MOVING;
    settings->mShape = m_shape;
    settings->mGravityFactor = 0.0f;
    settings->mFriction = 0.0f;
    settings->mSupportingVolume =
        JPH::Plane(JPH::Vec3::sAxisY(),
                   -(cylinder_params.half_height +
                       (1.0f - std::sinf(settings->mMaxSlopeAngle))));

    assert(s_physics_system != nullptr);
    assert(s_body_interface_ptr != nullptr);
    m_character_controller =
        new JPH::Character(settings,
                           position,
                           JPH::Quat::sIdentity(),
                           0,
                           s_physics_system);
    s_body_interface_ptr->SetMotionQuality(m_character_controller->GetBodyID(),
                                           JPH::EMotionQuality::Discrete);

    m_character_controller->AddToPhysicsSystem(JPH::EActivation::Activate);
}

phys_obj::Actor_character_controller::~Actor_character_controller()
{
    // @TODO: STUB
    assert(false);
}

void phys_obj::Actor_character_controller::set_position(JPH::RVec3Arg position)
{
    m_character_controller->SetPosition(position);
}

void phys_obj::Actor_character_controller::move(JPH::Vec3Arg velocity)
{
    m_character_controller->SetLinearVelocity(velocity);
}


// Helpers.
phys_obj::Shape_const_reference phys_obj::create_shape(Shape_type shape_type,
                                                       Shape_params_ptr shape_params)
{
    assert(shape_params != nullptr);
    Shape_const_reference shape{ nullptr };

    switch (shape_type)
    {
    case SHAPE_TYPE_BOX:
    {
        auto& params{
            *reinterpret_cast<Shape_params_box*>(
                shape_params) };
        shape =
            JPH::BoxShapeSettings(
                JPH::Vec3(params.half_x, params.half_y, params.half_z))
                .Create()
                .Get();
        break;
    }

    case SHAPE_TYPE_SPHERE:
    {
        auto& params{
            *reinterpret_cast<Shape_params_sphere*>(
                shape_params) };
        shape =
            JPH::SphereShapeSettings(params.radius)
                .Create()
                .Get();
        break;
    }

    case SHAPE_TYPE_CAPSULE:
    {
        auto& params{
            *reinterpret_cast<Shape_params_capsule*>(
                shape_params) };
        shape =
            JPH::CapsuleShapeSettings(params.half_height, params.radius)
                .Create()
                .Get();
        break;
    }

    case SHAPE_TYPE_TAPERED_CAPSULE:
    {
        auto& params{
            *reinterpret_cast<Shape_params_tapered_capsule*>(
                shape_params) };
        shape =
            JPH::TaperedCapsuleShapeSettings(
                params.half_height,
                params.top_radius,
                params.bottom_radius)
                .Create()
                .Get();
        break;
    }

    case SHAPE_TYPE_CYLINDER:
    {
        auto& params{
            *reinterpret_cast<Shape_params_cylinder*>(
                shape_params) };
        shape =
            JPH::CylinderShapeSettings(params.half_height, params.radius)
                .Create()
                .Get();
        break;
    }

    case SHAPE_TYPE_TAPERED_CYLINDER:
    {
        auto& params{
            *reinterpret_cast<Shape_params_tapered_cylinder*>(
                shape_params) };
        shape =
            JPH::TaperedCylinderShapeSettings(
                params.half_height,
                params.top_radius,
                params.bottom_radius)
                .Create()
                .Get();
        break;
    }

    default:
        assert(false);
        break;
    }

    return shape;
}

