use crate::rgb;

struct AnimationHandler {
    animation_fps: i32,
    keyboard: rgb::KeyboardData
}

pub trait Animation {
    fn update(&self);
    fn set_fps(&self);
    fn get_fps(&self) -> u32;
}