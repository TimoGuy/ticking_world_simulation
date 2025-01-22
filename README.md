# ticking_world_simulation
Component of HSGE. Simulation engine that powers the backend on a consistent ticking timeline.

## Object lifetime.
Ok ngl this is kinda weird af.

I was originally gonna have objects be created by the ticking world simulation and objects inherit the sim entity interface, but I don't think that's gonna work so well.

I think there's gonna have to be some way to connect the simulating entity to other stuff.
    - Objectttt
        - Adding a function to create the object into a string to function hash map.
            - Return the handle to the key to the start thingo thingy thing.
        - `simulating_entity_ifc.h` is used
        - ????? How does the objects get destroyed????
            - f
    - Hmmmmmmm..... I feel like these are the basic things I need to do:
        - Create some kind of object that implements the sim entity ifc. It could be some object that has a bunch of components like physics objects and stuff. Then,
        ```
            World_simulation world_sim{};
            world_sim.add_sim_entity(std::make_unique<Player>());
            world_sim.add_sim_entity(std::make_unique<Ground>());
            world_sim.add_sim_entity(std::make_unique<Ground>());
            world_sim.add_sim_entity(std::make_unique<Ground>());
        ```
        - So then when the job source gets created and added to the job system, the key that the sim entity gets assigned gets returned in `on_sim_entity_start(pool_elem_key_t)`. This gets used as a key to reference the entity and delete the entity.
            - Well, for inter-entity referencing, there needs to be usage with the guid thing.
        - So then maybe what I gotta do since there's like the `world_entity` stuff, I should:
        ```cpp
            class Player
                : public Simulating_entity_ifc
            {
            public:
                Player()
                    : m_world_entity(false)
                {
                }

                void on_sim_entity_start(pool_elem_key_t key) override
                {
                    m_sim_entity_key = key;
                    // Do stuff.
                }

                void tick_sim_entity() override
                {
                    // Do stuff.
                }

                void on_sim_entity_destroy() override
                {
                    // Do stuff.
                }
            private:
                pool_elem_key_t m_sim_entity_key;
                World_entity m_world_entity;
            };
        ```

