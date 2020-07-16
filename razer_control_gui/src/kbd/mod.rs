pub mod effects;
mod board;
use std::time::{SystemTime, UNIX_EPOCH};
use serde::{Deserialize, Serialize};
use serde_json::json;



const ANIMATION_FPS: u64 = 30; // 33 ms ~= 30fps

pub const ANIMATION_SLEEP_MS: u64 = (1000.0 / ANIMATION_FPS as f32) as u64;

pub fn get_millis() -> u128 {
    SystemTime::now()
    .duration_since(UNIX_EPOCH)
    .unwrap()
    .as_millis()
}

#[derive(Serialize, Deserialize)]
pub struct EffectSave {
    args: Vec<u8>,
    name: String,
}

/// Base effect trait.
/// An effect is a lighting function that is updated 30 times per seonc
/// in order to create an animation of some description on the laptop's
/// keyboard
pub trait Effect : Send + Sync {
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
    fn save(&mut self) -> EffectSave;
    fn get_state(&mut self) -> Vec<u8>;
}

/// An effect combined with a mask layer.
/// The mask layer tells the Effect Manager to apply the given
/// Effect to. This allows for stacked effects
struct EffectLayer {
    /// Mask for keys
    key_mask: Vec<bool>,
    effect: Box<dyn Effect>,
}

unsafe impl Send for EffectLayer {}
unsafe impl Sync for EffectLayer {}

impl EffectLayer {
    fn new(effect: Box<dyn Effect>, mask: [bool; 90]) -> EffectLayer {
        return EffectLayer {
            key_mask : mask.to_vec(),
            effect,
        }
    }

    fn update(&mut self) -> board::KeyboardData {
        return self.effect.update();
    }

    fn get_save(&mut self) -> Option<serde_json::Value> {
        match serde_json::to_value(&self.effect.save()) {
            Ok(mut x) => {
                let keys = serde_json::to_value(&self.key_mask).unwrap();
                x.as_object_mut()
                    .unwrap()
                    .insert(String::from("key_mask"), keys);
                Some(x)
            }
            Err(_) => None,
        }
    }

    fn from_save(mut json: serde_json::Value) -> Option<EffectLayer> {
        if json["key_mask"].is_null() || json["name"].is_null() || json["args"].is_null() {
            eprintln!("Missing data for effect!");
            return None;
        }
        let key_mask: Vec<bool> = serde_json::from_value(json["key_mask"].clone()).unwrap();
        if key_mask.len() != 90 {
            eprintln!(
                "Invalid key count effect. Expected 90, found {}",
                key_mask.len()
            );
            return None;
        }
        let name: String = serde_json::from_value(json["name"].clone()).unwrap();
        let args: Vec<u8> = serde_json::from_value(json["args"].clone()).unwrap();
        
        let effect: Option<Box<dyn Effect>> = match name.as_str() {
            "Static" => Some(effects::Static::new(args)),
            "Wave Gradient" => Some(effects::WaveGradient::new(args)),
            "Breathing Single" => Some(effects::BreathSingle::new(args)),
            _ => None
        };
        if effect.is_none() {
            eprintln!(
                "Effect failed to load. Invalid name: {}",
                name
            );
            return None;
        }
        return Some(EffectLayer {
            key_mask,
            effect: effect.unwrap()
        });
    }

    pub fn get_state(&mut self) -> Vec<u8> {
        self.effect.get_state()
    }

    pub fn get_mask(&mut self) -> Vec<bool> {
        self.key_mask.to_vec()
    }
}
pub struct EffectManager {
    layers: Vec<EffectLayer>,
    last_update_ms: u128,
    render_board: board::KeyboardData,
}

unsafe impl Send for EffectManager {}
unsafe impl Sync for EffectManager {}

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

    pub fn save(&mut self) -> serde_json::value::Value {
        let mut save_json = json!({"effects" : []});

        let tmp_saves : Vec<Option<serde_json::Value>> = self
        .layers
        .iter_mut()
        .map(|l| l.get_save())
        .collect();

        for save in tmp_saves {
            if let Some(x) = save {
                save_json["effects"].as_array_mut().unwrap().push(x);
            } else {
                eprintln!("Warning, discarding effect!");
            }
        }
        return save_json;
    }

    pub fn load_from_save(&mut self, mut json: serde_json::Value) {
        if json["effects"].is_null() {
            eprintln!("Invalid json. No effects field!");
            return;
        }
        for e in json["effects"].as_array_mut().unwrap() {
            if let Some(x) = EffectLayer::from_save(e.clone()) {
                self.layers.push(x);
            } else {
                eprintln!("Error adding effect");
            }
        }
    }

    pub fn get_map(&mut self, layer_id: i32) -> Vec<u8> {
        if layer_id < 0 { // Requesting global layer
            return self.render_board.get_curr_state();
        } else {
            return self.layers[layer_id as usize].get_state()
        }
    }
}
