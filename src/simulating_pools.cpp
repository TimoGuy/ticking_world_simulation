
#include <array>
#include "simulating_ifc.h"


namespace simulating
{

using Behavior_input_data_chunk =
    uint8_t[
        sizeof(uint32_t) +   // Bit 0: Pool elem version number.
        sizeof(float_t) * 3  // A vec3 (example benchmark data amount).
    ];

Data_pool<

}  // namespace simulating