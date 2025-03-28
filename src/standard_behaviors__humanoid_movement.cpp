#include "standard_behaviors.h"

#include "physics_objects.h"


// class Humanoid_movement.
std_behavior::Humanoid_movement::Humanoid_movement(
    phys_obj::Actor_character_controller& output_phys_char_ctrl)
    : m_output_phys_char_ctrl(output_phys_char_ctrl)
{
}

void std_behavior::Humanoid_movement::set_output(
    pool::elem_key_t output_animator_ctrl)
{
    m_output_animator_ctrl = output_animator_ctrl;
}

void std_behavior::Humanoid_movement::on_update()
{
    auto& input_data{
        get_data_from_input<Humanoid_movement_input_data>() };

    // input_data.
    // static_assert(false);  // @TODO: DO STUFF HERE!!!!

    // Send data.
    Humanoid_animator_input_data animator_data;
    animator_data.anim_state_packed = Humanoid_animator_input_data::WALKING;
    send_data_to_output<Humanoid_animator_input_data>(
        m_output_animator_ctrl, std::move(animator_data));

    // Send data.
    JPH::Vec3 delta_movement{
        input_data.flat_movement[0],
        1.0f,
        input_data.flat_movement[1]
    };
    m_output_phys_char_ctrl.move(delta_movement);
}
