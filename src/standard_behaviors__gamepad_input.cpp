#include "standard_behaviors.h"

#include "input_handling_public.h"


// class Gamepad_input_behavior.
std_behavior::Gamepad_input_behavior::Gamepad_input_behavior()
    : m_gamepad_idx(0)
    , m_prev_jump(false)
{
}

void std_behavior::Gamepad_input_behavior::set_output(pool::elem_key_t output_humanoid_mvt)
{
    m_output_humanoid_mvt = output_humanoid_mvt;
}

void std_behavior::Gamepad_input_behavior::on_update()
{
    auto& ih_handle{
        input_handling::get_state_set_reading_handle(m_gamepad_idx) };

    bool current_jump{ ih_handle.gameplay.jump };

    // Send data.
    Humanoid_movement_input_data data;
    glm_vec2_copy(const_cast<float_t*>(ih_handle.gameplay.movement),
                  data.flat_movement);
    data.start_jump = (current_jump && !m_prev_jump);
    data.release_jump = (!current_jump && m_prev_jump);
    send_data_to_output<Humanoid_movement_input_data>(
        m_output_humanoid_mvt, std::move(data));
    
    // Update prev.
    m_prev_jump = current_jump;
}
