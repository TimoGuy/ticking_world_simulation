#include "physics_objects.h"

#include "cglm/cglm.h"
#include "jolt_physics_headers.h"
#include "jolt_phys_impl__layers.h"
#include "world_simulation_settings.h"

#include <thread>


namespace phys_obj
{

static JPH::PhysicsSystem* s_physics_system{ nullptr };
static JPH::BodyInterface* s_body_interface_ptr{ nullptr };
static JPH::JobSystem* s_job_system_ptr{ nullptr };

Shape_const_reference create_shape(Shape_type shape_type,
                                   Shape_params_ptr shape_param);

}  // namespace phys_obj


// References.
void phys_obj::set_references(void* physics_system, void* body_interface, void* job_system)
{
    s_physics_system = reinterpret_cast<JPH::PhysicsSystem*>(physics_system);
    s_body_interface_ptr = reinterpret_cast<JPH::BodyInterface*>(body_interface);
    s_job_system_ptr = reinterpret_cast<JPH::JobSystem*>(job_system);
}

// Transform_holder.
phys_obj::Transform_holder::Transform_holder(
    bool interpolate,
    const Query_physics_transform_ifc& physics_transform_ref)
    : m_interpolate_transform(interpolate)
    , m_physics_transform_ref(physics_transform_ref)
{
    auto initial_transform{ m_physics_transform_ref.query_physics_transform() };
    for (size_t i = 0; i < k_num_buffers; i++)
    {
        m_transform_triple_buffer[i].position[0] = initial_transform.position[0];
        m_transform_triple_buffer[i].position[1] = initial_transform.position[1];
        m_transform_triple_buffer[i].position[2] = initial_transform.position[2];
        glm_quat_copy(initial_transform.rotation, m_transform_triple_buffer[i].rotation);
        glm_vec3_copy(initial_transform.scale, m_transform_triple_buffer[i].scale);
    }
}

void phys_obj::Transform_holder::update_physics_transform()
{
    auto transform{ m_physics_transform_ref.query_physics_transform() };

    auto& current_transform{
        m_transform_triple_buffer[(m_buffer_offset + k_write_offset) % k_num_buffers] };

    current_transform.position[0] = transform.position[0];
    current_transform.position[1] = transform.position[1];
    current_transform.position[2] = transform.position[2];
    glm_quat_copy(transform.rotation, current_transform.rotation);
    glm_vec3_copy(transform.scale, current_transform.scale);
}

void phys_obj::Transform_holder::read_current_transform(mat4& out_transform, float_t t)
{
    // @TODO: In the future with world chunks and camera tricks for rendering large worlds,
    //   convert the true transforms into the camera based transform here!
    //   For now tho, it all just gets truncated.

    vec3   pos;
    versor rot;
    vec3   sca;

    size_t buffer_offset_copy{ m_buffer_offset };  // To only do one atomic load.

    if (m_interpolate_transform)
    {
        auto& transform_a{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_a_offset) % k_num_buffers] };
        auto& transform_b{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_b_offset) % k_num_buffers] };

        // @INCOMPLETE: Truncated data (if using double real JPH::RVec3).
        pos[0] = transform_a.position[0] + t * (transform_b.position[0] - transform_a.position[0]);
        pos[1] = transform_a.position[1] + t * (transform_b.position[1] - transform_a.position[1]);
        pos[2] = transform_a.position[2] + t * (transform_b.position[2] - transform_a.position[2]);

        glm_quat_nlerp(transform_a.rotation, transform_b.rotation, t, rot);

        glm_vec3_lerp(transform_a.scale, transform_b.scale, t, sca);
    }
    else
    {
        auto& transform_b{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_b_offset) % k_num_buffers] };

        // @INCOMPLETE: Truncated data (if using double real JPH::RVec3).
        pos[0] = transform_b.position[0];
        pos[1] = transform_b.position[1];
        pos[2] = transform_b.position[2];

        glm_quat_copy(transform_b.rotation, rot);

        glm_vec3_copy(transform_b.scale, sca);
    }

    // Write transform.
    glm_translate_make(out_transform, pos);
    glm_quat_rotate(out_transform, rot, out_transform);
    glm_scale(out_transform, sca);
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
    assert(s_job_system_ptr != nullptr);
    JPH::JobHandle handle =
        s_job_system_ptr->CreateJob("CreateAndAddBody", JPH::ColorArg::sGreen, [&]() {
            m_body_id =
                s_body_interface_ptr->CreateAndAddBody(
                    JPH::BodyCreationSettings(m_shape,
                                            position,
                                            rotation,
                                            JPH::EMotionType::Kinematic,
                                            Layers::MOVING),
                    JPH::EActivation::Activate);
        });
    while (!handle.IsDone())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

phys_obj::Actor_kinematic::~Actor_kinematic()
{
    // @NOTE: This may be not quite the right thing to do, however, this is how
    //   I'm choosing to deal with it.
    //   Essentially, the shape ref should still be connected if this is the
    //   owning object. If it is, then it's responsible for removing the physics
    //   body.  -Thea 2025/03/31
    if (m_shape != nullptr)
    {
        s_body_interface_ptr->RemoveBody(m_body_id);
    }
}

void phys_obj::Actor_kinematic::get_position_and_rotation(JPH::RVec3& out_position,
                                                          JPH::Quat& out_rotation) const
{
    s_body_interface_ptr->GetPositionAndRotation(m_body_id,
                                                 out_position,
                                                 out_rotation);
}

void phys_obj::Actor_kinematic::set_transform(JPH::RVec3Arg position, JPH::QuatArg rotation)
{
    s_body_interface_ptr->SetPositionAndRotation(m_body_id,
                                                 position,
                                                 rotation,
                                                 JPH::EActivation::DontActivate);
}

void phys_obj::Actor_kinematic::move_kinematic(JPH::RVec3Arg position, JPH::QuatArg rotation)
{
    s_body_interface_ptr->MoveKinematic(m_body_id,
                                        position,
                                        rotation,
                                        k_world_sim_delta_time);
}

phys_obj::Transform_decomposed phys_obj::Actor_kinematic::query_physics_transform() const
{
    JPH::RVec3 position;
    JPH::Quat rotation;
    get_position_and_rotation(position, rotation);

    return {
        .position{ position.GetX(), position.GetY(), position.GetZ() },
        .rotation{ rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() },
        .scale{ 1.0f, 1.0f, 1.0f },
    };
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
    // @NOTE: Move constructor nullifies character controller, so only owning
    //   wrapper will remove the character controller from the physics system.
    if (m_character_controller != nullptr)
    {
        m_character_controller->RemoveFromPhysicsSystem();
    }
}

void phys_obj::Actor_character_controller::set_position(JPH::RVec3Arg position)
{
    m_character_controller->SetPosition(position);
}

void phys_obj::Actor_character_controller::move(JPH::Vec3Arg velocity)
{
    m_character_controller->SetLinearVelocity(velocity);
}

phys_obj::Transform_decomposed phys_obj::Actor_character_controller::query_physics_transform() const
{
    // @NOTE: I thought that `GetPosition` would be quicker/lighter than
    //   `GetCenterOfMassPosition`, but getting the position negates the center
    //   of mass, thus causing an extra subtract operation.  -Thea 2023/09/28
    JPH::RVec3 position{ m_character_controller->GetCenterOfMassPosition() };

    return {
        .position{ position.GetX(), position.GetY(), position.GetZ() },
        .rotation{ 0.0f, 0.0f, 0.0f, 1.0f },
        .scale{ 1.0f, 1.0f, 1.0f },
    };
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

