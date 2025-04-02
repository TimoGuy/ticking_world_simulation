#include "standard_behaviors.h"

#include "physics_objects.h"


// class Kinematic_collider.
std_behavior::Kinematic_collider::Kinematic_collider(
    phys_obj::Actor_kinematic&& phys_kinematic_actor)
    : m_phys_kinematic_actor(std::move(phys_kinematic_actor))
{
}

void std_behavior::Kinematic_collider::on_update()
{
    auto& input_data{
        get_data_from_input<Kinematic_collider_transform_input_data>() };

    switch (input_data.type)
    {
    case TRANS_DATA_TYPE_NONE:
        // Do nothing.
        break;

    case TRANS_DATA_TYPE_SET:
        m_phys_kinematic_actor.set_transform(input_data.position,
                                             input_data.rotation);
        break;

    case TRANS_DATA_TYPE_MOVE_ABSOLUTE:
        m_phys_kinematic_actor.move_kinematic(input_data.position,
                                              input_data.rotation);
        break;

    case TRANS_DATA_TYPE_MOVE_DELTA:
    {
        JPH::RVec3 prev_position;
        JPH::Quat  prev_rotation;
        m_phys_kinematic_actor.get_position_and_rotation(prev_position, prev_rotation);
        m_phys_kinematic_actor.move_kinematic(prev_position + input_data.position,
                                              prev_rotation * input_data.rotation);
        break;
    }

    default:
        assert(false);
        break;
    }
}
