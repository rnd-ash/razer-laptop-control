
mod rgb;



pub struct MachineState {
    pub power_mode: u8,
    pub fan_rpm: u8,
    pub kboard: rgb::keyboard
}