#include "standard_behaviors.h"


// class Humanoid_movement.
std_behavior::Humanoid_movement::Humanoid_movement()
{
}

void std_behavior::Humanoid_movement::on_update()
{
    auto& input_data{
        get_data_from_input<Humanoid_movement_input_data>() };

    // input_data.
    static_assert(false);  // @TODO: DO STUFF HERE!!!!

    // Send data.
    Character_controller_input_data data;
    data.delta_movement[0] = input_data.flat_movement[0];
    data.delta_movement[0] = 1.0f;
    data.delta_movement[2] = input_data.flat_movement[1];
    send_data_to_output<Character_controller_input_data>(std::move(data));
}
