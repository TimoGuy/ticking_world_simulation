#pragma once

#include "cglm/cglm.h"
#include "simulating_ifc.h"


namespace std_behavior
{

class Gamepad_input_behavior
    : public simulating::Behavior_ifc
{
public:
    Gamepad_input_behavior();

    void on_update() override;

private:
    uint32_t m_gamepad_idx;
    bool m_prev_jump;
};

struct Humanoid_movement_input_data
{
    vec2 flat_movement;
    bool start_jump;
    bool release_jump;
};

class Humanoid_movement
    : public simulating::Behavior_ifc
{
public:
    Humanoid_movement();

    void on_update() override;
};

struct Character_controller_input_data
{
    vec3 delta_movement;
};

}  // namespace std_behavior

