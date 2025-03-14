#pragma once

#include "jolt_physics_headers.h"


// An example activation listener
class My_body_activation_listener : public JPH::BodyActivationListener
{
public:
    virtual void OnBodyActivated(const JPH::BodyID& in_body_id, uint64_t in_body_user_data) override
    {
        //std::cout << "A body got activated" << std::endl;
    }

    virtual void OnBodyDeactivated(const JPH::BodyID& in_body_id, uint64_t in_body_user_data) override
    {
        //std::cout << "A body went to sleep" << std::endl;
    }
};


// An example contact listener
class My_contact_listener : public JPH::ContactListener
{
public:
    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& in_body1, const JPH::Body& in_body2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
    {
        //std::cout << "Contact validate callback" << std::endl;

        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void process_user_data_meaning(const JPH::Body& this_body, const JPH::Body& other_body, const JPH::ContactManifold& manifold, JPH::ContactSettings& io_settings)
    {
        // @TODO: Implement as needed!

        // switch (UserDataMeaning(this_body.GetUserData()))
        // {
        // case UserDataMeaning::NOTHING:
        //     break;
        
        // case UserDataMeaning::IS_CHARACTER:
        // {
        //     SimulationCharacter* entityAsChar;
        //     if (entityAsChar = dynamic_cast<SimulationCharacter*>(entityManager->getEntityViaGUID(bodyIdToEntityGuidMap[this_body.GetID()])))
        //         entityAsChar->reportPhysicsContact(other_body, manifold, &io_settings);
        //     break;
        // }
        // }
    }

    virtual void OnContactAdded(const JPH::Body& in_body1,
                                const JPH::Body& in_body2,
                                const JPH::ContactManifold& in_manifold,
                                JPH::ContactSettings& io_settings) override
    {
        //std::cout << "A contact was added" << std::endl;
        process_user_data_meaning(in_body1, in_body2, in_manifold, io_settings);
        process_user_data_meaning(in_body2, in_body1, in_manifold.SwapShapes(), io_settings);
    }

    virtual void OnContactPersisted(const JPH::Body& in_body1, const JPH::Body& in_body2, const JPH::ContactManifold& in_manifold, JPH::ContactSettings& io_settings) override
    {
        //std::cout << "A contact was persisted" << std::endl;
        process_user_data_meaning(in_body1, in_body2, in_manifold, io_settings);
        process_user_data_meaning(in_body2, in_body1, in_manifold.SwapShapes(), io_settings);
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        //std::cout << "A contact was removed" << std::endl;
    }
};
