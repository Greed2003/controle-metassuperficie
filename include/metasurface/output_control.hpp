#ifndef METASURFACE_OUTPUT_CONTROL_HPP
#define METASURFACE_OUTPUT_CONTROL_HPP

namespace metasurface {

class ControllerState;
class DigitalOutput;

void initialize_outputs(DigitalOutput& output);
void apply_outputs(const ControllerState& state, DigitalOutput& output);

}

#endif
