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

    // @TODO: STUB.
    assert(false);
}
