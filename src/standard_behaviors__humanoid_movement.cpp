#include "standard_behaviors.h"


// class Humanoid_movement.
std_behavior::Humanoid_movement::Humanoid_movement()
{
}

void std_behavior::Humanoid_movement::set_output(
    pool::elem_key_t output_animator_ctrl,
    pool::elem_key_t output_phys_char_ctrl)
{
    m_output_animator_ctrl = output_animator_ctrl;
    m_output_phys_char_ctrl = output_phys_char_ctrl;
}

void std_behavior::Humanoid_movement::on_update()
{
    auto& input_data{
        get_data_from_input<Humanoid_movement_input_data>() };

    // input_data.
    static_assert(false);  // @TODO: DO STUFF HERE!!!!

    // Send data.
    Humanoid_animator_input_data data1;
    data1.anim_state_packed = Humanoid_animator_input_data::WALKING;
    send_data_to_output<Humanoid_animator_input_data>(
        m_output_animator_ctrl, std::move(data1));

    // Send data.
    Character_controller_input_data data2;
    data2.delta_movement[0] = input_data.flat_movement[0];
    data2.delta_movement[0] = 1.0f;
    data2.delta_movement[2] = input_data.flat_movement[1];
    send_data_to_output<Character_controller_input_data>(
        m_output_phys_char_ctrl, std::move(data2));
}
