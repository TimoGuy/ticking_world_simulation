#pragma once

#if 0

#include <cinttypes>
#include <functional>


using pool_elem_key_t = uint64_t;
using Component_entry_fn = std::function<void()>;

// @NOTE: Customizable pipeline of components. Expected usage is to use each as an object.
struct Simulating_object_callback_set
{
    // World simulation events.
    Component_entry_fn on_create_callback{ nullptr };
    Component_entry_fn on_destroy_callback{ nullptr };
    Component_entry_fn on_sim_logic_update_callback{ nullptr };  // Pre-physics update event.

    // Renderer query.
    Component_entry_fn on_renderer_transform_query_callback{ nullptr };
};

#endif  // 0
