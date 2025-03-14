#pragma once


namespace phys_obj
{

// Shapes.
class Shape_box
{
public:

};

class Shape_capsule
{
public:

};

class Shape_cylinder
{
public:

};

class Shape_sphere
{
public:

};

class Shape_tapered_capsule
{
public:

};

class Shape_tapered_cylinder
{
public:

};


// Actors.
class Actor_kinematic
{

};

class Actor_character_controller
{
// @NOTE: A cylinder shape character controller with collide-and-slide functionality and moving platform, stair climbing features.

};

// Hitboxes.
class Hitbox_skeletal_bone
{
// @NOTE: A collection of hitboxes connected to a skeletal animation armature.

};

class Hitbox_kinematic
{
// @NOTE: A non-solid region where hurtboxes are trying to be detected. Could be a switch or something.

};

// Hurtboxes.
class Hurtbox_blade
{
// @NOTE: Connects to a bone of the skeletal animator and calculates the delta orientations to find the way the blade traveled. Calculates a dynamic hurtbox from this.

};

class Hurtbox_kinematic
{
// @NOTE: Essentially just a hazard.

};

// Trigger.
class Trigger_kinematic
{

};

}  // namespace phys_obj
