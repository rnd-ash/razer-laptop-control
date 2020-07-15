pub mod effects;
mod board;
use std::time::{SystemTime, UNIX_EPOCH};

const ANIMATIONS_DELAY_MS: u128 = 33; // 33 ms ~= 30fps

pub fn get_millis() -> u128 {
    SystemTime::now()
    .duration_since(UNIX_EPOCH)
    .unwrap()
    .as_millis()
}

/// Base effect trait.
/// An effect is a lighting function that is updated 30 times per seonc
/// in order to create an animation of some description on the laptop's
/// keyboard
pub trait Effect {
    /// Returns a new instance of an Effect
    fn new(args: Vec<u8>) -> Box<dyn Effect> where Self: Sized;
    /// Updates the keyboard, returning the current state of the keyboard
    /// Called 30 times per second by the Effect Manager
    fn update(&mut self) -> board::KeyboardData;
    /// Returns the arguments used to spawn the effect
    fn get_varargs(&mut self) -> &[u8];
    /// Returns the name of the effect (Unique identifier)
    fn get_name() -> &'static str where Self: Sized;
    fn clone_box(&self) -> Box<dyn Effect>;
}

/// An effect combined with a mask layer.
/// The mask layer tells the Effect Manager to apply the given
/// Effect to. This allows for stacked effects
struct EffectLayer {
    /// Mask for keys
    key_mask: [bool; 90],
    effect: Box<dyn Effect>,
}

impl EffectLayer {
    fn new(effect: Box<dyn Effect>, key_mask: [bool; 90]) -> EffectLayer {
        return EffectLayer {
            key_mask,
            effect,
        }
    }

    fn update(&mut self) -> board::KeyboardData {
        return self.effect.update();
    }
}
pub struct EffectManager {
    layers: Vec<EffectLayer>,
    last_update_ms: u128,
    render_board: board::KeyboardData,
}

unsafe impl Send for EffectManager {}

impl EffectManager {
    pub fn new() -> EffectManager {
        EffectManager {
            layers: vec![],
            last_update_ms : get_millis(),
            render_board: board::KeyboardData::new(),
        }
    }

    pub fn push_effect(&mut self, effect: Box<dyn Effect>, mask: [bool; 90]) {
        self.layers.push(EffectLayer::new(effect, mask))
    }

    pub fn pop_effect(&mut self) {
        self.layers.pop();
    }

    pub fn update(&mut self) {
        for layer in self.layers.iter_mut() {
            let tmp_board = layer.update();
            for (pos, state) in layer.key_mask.iter().enumerate() {
                if *state == true {
                    self.render_board.set_key_at(pos, tmp_board.get_key_at(pos))
                }
            }
        }
        // Don't forget to actually render the board
        self.last_update_ms = get_millis();
        self.render_board.update_kbd();
    }
}
