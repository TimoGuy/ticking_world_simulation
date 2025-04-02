#pragma once

#include "cglm/cglm.h"
#include "physics_objects.h"
#include "simulating_ifc.h"


namespace std_behavior
{

class Gamepad_input_behavior
    : public simulating::Behavior_ifc
{
public:
    Gamepad_input_behavior();

    void set_output(pool::elem_key_t output_humanoid_mvt);

    void on_update() override;

private:
    uint32_t m_gamepad_idx;
    bool m_prev_jump;

    pool::elem_key_t m_output_humanoid_mvt;
};

// Physics wrappers.
enum Transform_data_type : uint8_t
{
    TRANS_DATA_TYPE_NONE = 0,
    TRANS_DATA_TYPE_SET,  // i.e. no interpolation with this.
    TRANS_DATA_TYPE_MOVE_ABSOLUTE,
    TRANS_DATA_TYPE_MOVE_DELTA,

    NUM_TRANS_DATA_TYPES
};

struct Kinematic_collider_transform_input_data
{
    Transform_data_type type;
    JPH::RVec3          position;
    JPH::Quat           rotation;
};

class Kinematic_collider
    : public simulating::Behavior_ifc
{
public:
    Kinematic_collider(phys_obj::Actor_kinematic&& phys_kinematic_actor);

    void on_update() override;

private:
    phys_obj::Actor_kinematic m_phys_kinematic_actor;
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
    Humanoid_movement(
        phys_obj::Actor_character_controller&& phys_char_ctrl);

    void set_animator(pool::elem_key_t output_animator_ctrl);

    void on_update() override;

private:
    phys_obj::Actor_character_controller m_phys_char_ctrl;
    pool::elem_key_t m_output_animator_ctrl;
};

struct Humanoid_animator_input_data
{
    // Humanoid anim states.
    static constexpr uint32_t k_mask_movement = 0x000000ff;
    static constexpr uint32_t MVT_INVALID     = 0x00000000;
    static constexpr uint32_t IDLE            = 0x00000001;
    static constexpr uint32_t WALKING         = 0x00000002;
    static constexpr uint32_t RUNNING         = 0x00000003;
    static constexpr uint32_t JUMP_UP         = 0x00000004;
    static constexpr uint32_t FALL_DOWN       = 0x00000005;
    static constexpr uint32_t k_mask_combat   = 0x0000ff00;
    static constexpr uint32_t COMBAT_NONE     = 0x00000000;
    static constexpr uint32_t CHARGE_ATTACK   = 0x00000100;

    uint32_t anim_state_packed;
};

class Humanoid_animator
    : public simulating::Behavior_ifc
{
public:
    void on_update() override {}  // @NOCHECKIN @WIP.
};

}  // namespace std_behavior

