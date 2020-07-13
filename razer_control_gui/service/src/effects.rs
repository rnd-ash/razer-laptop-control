use crate::daemon_core;
use crate::rgb;
use serde::{Deserialize, Serialize};
use serde_json::json;
use std::time::{SystemTime, UNIX_EPOCH};

const ANIMATIONS_DELAY_MS: u128 = 33; // 33 ms ~= 30fps

/// Contains detail about a layer effect on the keyboard
pub struct EffectLayer {
    // Keys that are enabled or disabled for the effect layer
    effect_keys: Vec<bool>,
    // Keyboard effect
    effect: Box<dyn Effect>,
}

impl EffectLayer {
    fn new(keys_allowed: [bool; 90], effect: Box<dyn Effect>) -> EffectLayer {
        EffectLayer {
            effect_keys: keys_allowed.to_vec(),
            effect,
        }
    }

    /// Updates the effect layer, returning a keyboard struct of what the layer
    /// looks like on the keyboard once the update is complete
    fn update(&mut self) -> rgb::KeyboardData {
        self.effect.update()
    }

    fn save_to_json(&self) -> Option<serde_json::Value> {
        match serde_json::to_value(&self.effect.get_settings()) {
            Ok(mut x) => {
                let keys = serde_json::to_value(&self.effect_keys).unwrap();
                x.as_object_mut()
                    .unwrap()
                    .insert(String::from("keys"), keys);
                Some(x)
            }
            Err(_) => None,
        }
    }

    fn from_save(mut json: serde_json::Value) -> Option<EffectLayer> {
        if json["keys"].is_null() || json["name"].is_null() || json["settings"].is_null() {
            eprintln!("Missing data for effect!");
            return None;
        }
        let keys: Vec<bool> = serde_json::from_value(json["keys"].clone()).unwrap();
        if keys.len() != 90 {
            eprintln!(
                "Invalid key count effect. Expected 90, found {}",
                keys.len()
            );
            return None;
        }
        let name: String = serde_json::from_value(json["name"].clone()).unwrap();
        let settings: Vec<u32> = serde_json::from_value(json["settings"].clone()).unwrap();
        let mut effect = EffectSave { name, settings };
        if let Some(e) = effect.save_to_effect() {
            return Some(EffectLayer {
                effect_keys: keys,
                effect: e,
            });
        }
        None
    }
}

impl Clone for EffectLayer {
    fn clone(&self) -> Self {
        EffectLayer::from_save(self.save_to_json().unwrap()).unwrap()
    }
}

impl Copy for EffectLayer {}

pub struct EffectManager {
    layers: Vec<EffectLayer>,
    last_update_time: u128,
    render_board: rgb::KeyboardData, // Actual rendered keyboard
}

impl EffectManager {
    fn get_millis() -> u128 {
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis()
    }

    pub fn new() -> EffectManager {
        EffectManager {
            layers: vec![],
            last_update_time: SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_millis(),
            render_board: rgb::KeyboardData::new(),
        }
    }

    pub fn update(&mut self, handler: &mut daemon_core::DriverHandler) {
        if EffectManager::get_millis() - self.last_update_time >= ANIMATIONS_DELAY_MS {
            if self.layers.len() == 0 {
                return;
            } // Return if we have no effects!
              // Update all our effects
              // Create a temp map of keyboard
            for mut layer in self.layers.iter_mut() {
                let mut k = layer.update();
                for (pos, key) in layer.effect_keys.iter().enumerate() {
                    if *key == true {
                        self.render_board.set_key_at(pos, k.get_key_at(pos));
                    }
                }
            }
            &self.render_board.update_kbd(handler); // Render keyboard
            self.last_update_time = EffectManager::get_millis();
        }
    }

    pub fn get_effect_layer_count(&mut self) -> usize {
        self.layers.len()
    }

    pub fn push_effect(&mut self, new_effect: Box<dyn Effect>, enabled_keys: &[bool; 90]) {
        self.layers
            .push(EffectLayer::new(*enabled_keys, new_effect))
    }

    pub fn pop_effect(&mut self) {
        self.layers.pop();
    }

    pub fn get_save(&self) -> Option<serde_json::Value> {
        let tmp: Vec<Option<serde_json::Value>> = self
            .layers
            .clone()
            .iter_mut()
            .map(|l| l.save_to_json())
            .filter(|e| e.is_some())
            .collect();

        let mut save = json!({"effects": []});

        for e in tmp {
            save["effects"].as_array_mut().unwrap().push(e.unwrap());
        }
        Some(save)
    }

    pub fn from_save(mut json: serde_json::Value) -> Option<EffectManager> {
        if json["effects"].is_null() {
            return None;
        }; // Not our json!
        let mut res: Vec<EffectLayer> = vec![];
        for e in json["effects"].as_array_mut().unwrap() {
            if let Some(x) = EffectLayer::from_save(e.clone()) {
                res.push(x);
            }
        }
        Some(EffectManager {
            layers: res,
            last_update_time: SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_millis(),
            render_board: rgb::KeyboardData::new(),
        })
    }
}

impl Clone for EffectManager {
    fn clone(&self) -> Self {
        EffectManager {
            layers: self.layers.clone(),
            last_update_time: self.last_update_time,
            render_board: self.render_board.clone(),
        }
    }
}

#[derive(Copy, Clone)]
pub enum EffectDir {
    Vertical = 0,
    Horizontal = 1,
    Diagonal = 2,
    Circular = 3,
}

#[derive(Serialize, Deserialize)]
pub struct EffectSave {
    name: String,
    settings: Vec<u32>,
}

impl EffectSave {
    fn save_to_effect(&mut self) -> Option<Box<dyn Effect>> {
        match self.name.as_str() {
            "Static" => StaticEffect::from_settings(self.settings.clone()),
            "StaticBlend" => BlendEffect::from_settings(self.settings.clone()),
            "Breath1Colour" => BreathEffect::from_settings(self.settings.clone()),
            _ => None,
        }
    }
}

pub trait Effect: Send {
    fn get_settings(&self) -> EffectSave;
    fn from_settings(args: Vec<u32>) -> Option<Box<dyn Effect>>
    where
        Self: Sized;
    fn get_name() -> String
    where
        Self: Sized;
    fn update(&mut self) -> rgb::KeyboardData;
}

// -- Static effect code --
#[derive(Copy, Clone)]
pub struct StaticEffect {
    kbd: rgb::KeyboardData,
    colour: rgb::KeyColour,
}

impl StaticEffect {
    pub fn new(red: u8, green: u8, blue: u8) -> StaticEffect {
        let mut k = rgb::KeyboardData::new();
        k.set_kbd_colour(red, green, blue);
        StaticEffect {
            kbd: k,
            colour: rgb::KeyColour { red, green, blue },
        }
    }
}

impl Effect for StaticEffect {
    fn update(&mut self) -> rgb::KeyboardData {
        // Does nothing on static effect
        return self.kbd;
    }
    fn get_settings(&self) -> EffectSave {
        return EffectSave {
            name: String::from("Static"),
            settings: vec![
                self.colour.red as u32,
                self.colour.green as u32,
                self.colour.blue as u32,
            ],
        };
    }

    fn from_settings(args: Vec<u32>) -> Option<Box<dyn Effect>> {
        if args.len() != 3 {
            return None;
        }
        return Some(Box::new(StaticEffect::new(
            args[0] as u8,
            args[1] as u8,
            args[2] as u8,
        )));
    }

    fn get_name() -> String {
        String::from("Static")
    }
}

// -- 'Blend' effect code --
#[derive(Copy, Clone)]
pub struct BlendEffect {
    pub kbd: rgb::KeyboardData,
    colour1: rgb::KeyColour,
    colour2: rgb::KeyColour,
    dir: EffectDir,
}

impl BlendEffect {
    pub fn new(r1: u8, g1: u8, b1: u8, r2: u8, g2: u8, b2: u8, dir: EffectDir) -> BlendEffect {
        let mut k = rgb::KeyboardData::new();
        let dr: f32 = r2 as f32 - r1 as f32; // Delta red
        let dg: f32 = g2 as f32 - g1 as f32; // Delta green
        let db: f32 = b2 as f32 - b1 as f32; // Delta blue
        match dir {
            EffectDir::Vertical => {
                for x in 0..6 {
                    let col_blend_ratio = (x + 1) as f32 / 6.0;
                    k.set_row_colour(
                        x,
                        (r1 as f32 + (dr * col_blend_ratio)) as u8,
                        (g1 as f32 + (dg * col_blend_ratio)) as u8,
                        (b1 as f32 + (db * col_blend_ratio)) as u8,
                    );
                }
            }
            EffectDir::Horizontal => {
                for x in 0..15 {
                    let col_blend_ratio = (x + 1) as f32 / 15.0;
                    k.set_col_colour(
                        x,
                        (r1 as f32 + (dr * col_blend_ratio)) as u8,
                        (g1 as f32 + (dg * col_blend_ratio)) as u8,
                        (b1 as f32 + (db * col_blend_ratio)) as u8,
                    );
                }
            }
            _ => {
                // Unsupported direction, default to vertical
                eprintln!("BlendMode Diagonal unsupported, using vertical");
                return BlendEffect::new(r1, g1, b1, r2, g2, b2, EffectDir::Vertical);
            }
        }
        BlendEffect {
            kbd: k,
            colour1: rgb::KeyColour {
                red: r1,
                green: g1,
                blue: g1,
            },
            colour2: rgb::KeyColour {
                red: r2,
                green: g2,
                blue: g2,
            },
            dir,
        }
    }
}

impl Effect for BlendEffect {
    fn update(&mut self) -> rgb::KeyboardData {
        // Does nothing on static effect
        return self.kbd;
    }
    fn get_settings(&self) -> EffectSave {
        return EffectSave {
            name: String::from("StaticBlend"),
            settings: vec![
                self.colour1.red as u32,
                self.colour1.green as u32,
                self.colour1.blue as u32,
                self.colour2.red as u32,
                self.colour2.green as u32,
                self.colour2.blue as u32,
                self.dir as u32,
            ],
        };
    }

    fn from_settings(args: Vec<u32>) -> Option<Box<dyn Effect>> {
        if args.len() != 7 {
            return None;
        }
        let dir: EffectDir = match args[6] {
            0 => EffectDir::Vertical,
            1 => EffectDir::Horizontal,
            2 => EffectDir::Diagonal,
            3 => EffectDir::Circular,
            _ => EffectDir::Vertical, // Error out, so just make vertical
        };
        return Some(Box::new(BlendEffect::new(
            args[0] as u8,
            args[1] as u8,
            args[2] as u8,
            args[3] as u8,
            args[4] as u8,
            args[5] as u8,
            dir,
        )));
    }

    fn get_name() -> String {
        String::from("StaticBlend")
    }
}

// -- 'Breathing' effect
#[derive(Copy, Clone)]
pub struct BreathEffect {
    pub kbd: rgb::KeyboardData,
    step_duration_ms: u128,
    static_start_ms: u128,
    curr_step: u8, // Step 0 = Off, 1 = increasing, 2 = On, 3 = decreasing
    target_colour: rgb::AnimatorKeyColour,
    current_colour: rgb::AnimatorKeyColour,
    animator_step_colour: rgb::AnimatorKeyColour,
}

impl BreathEffect {
    pub fn new(red: u8, green: u8, blue: u8, cycle_duration_ms: u32) -> BreathEffect {
        let mut k = rgb::KeyboardData::new();
        k.set_kbd_colour(0, 0, 0); // Sets all keyboard lights off initially
        return BreathEffect {
            kbd: k,
            step_duration_ms: cycle_duration_ms as u128,
            static_start_ms: EffectManager::get_millis(),
            curr_step: 0,
            target_colour: rgb::AnimatorKeyColour {
                red: red as f32,
                green: green as f32,
                blue: blue as f32,
            },
            current_colour: rgb::AnimatorKeyColour {
                red: 0.0,
                green: 0.0,
                blue: 0.0,
            },
            animator_step_colour: rgb::AnimatorKeyColour {
                red: red as f32 / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
                green: green as f32
                    / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
                blue: blue as f32 / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
            },
        };
    }
}

impl Effect for BreathEffect {
    fn update(&mut self) -> rgb::KeyboardData {
        match self.curr_step {
            0 => {
                self.current_colour = rgb::AnimatorKeyColour {
                    red: 0.0,
                    green: 0.0,
                    blue: 0.0,
                };
                if EffectManager::get_millis() - self.static_start_ms >= self.step_duration_ms {
                    self.curr_step += 1;
                }
            }
            1 => {
                // Increasing
                self.current_colour += self.animator_step_colour;
                if self.current_colour >= self.target_colour {
                    self.curr_step += 1;
                    self.static_start_ms = EffectManager::get_millis();
                }
            }
            2 => {
                self.current_colour = self.target_colour;
                if EffectManager::get_millis() - self.static_start_ms >= self.step_duration_ms {
                    self.curr_step += 1;
                }
            }
            3 => {
                // Decreasing
                self.current_colour -= self.animator_step_colour;
                let target = rgb::AnimatorKeyColour {
                    red: 0.0,
                    green: 0.0,
                    blue: 0.0,
                };
                if self.current_colour <= target {
                    self.curr_step = 0;
                    self.static_start_ms = EffectManager::get_millis();
                }
            }
            _ => {} // Unknown state? Ignore
        }
        let col = self.current_colour.get_clamped_colour();
        self.kbd.set_kbd_colour(col.red, col.green, col.blue); // Cast back to u8
        return self.kbd;
    }

    fn get_settings(&self) -> EffectSave {
        let c = self.target_colour.get_clamped_colour();
        return EffectSave {
            name: String::from("Breath1Colour"),
            settings: vec![
                self.step_duration_ms as u32,
                c.red as u32,
                c.green as u32,
                c.blue as u32,
            ],
        };
    }

    fn from_settings(args: Vec<u32>) -> Option<Box<dyn Effect>> {
        if args.len() != 4 {
            return None;
        }
        return Some(Box::new(BreathEffect::new(
            args[1] as u8,
            args[2] as u8,
            args[3] as u8,
            args[0],
        )));
    }

    fn get_name() -> String {
        String::from("Breath1Colour")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_save() {
        let effect1: Box<dyn Effect> = Box::new(BreathEffect::new(80, 70, 60, 2000));
        let effect2: Box<dyn Effect> = Box::new(StaticEffect::new(255, 255, 255));
        let mut manager = EffectManager::new();
        manager.push_effect(effect1, &[true; 90]);
        manager.push_effect(effect2, &[false; 90]);
        let save = manager.get_save().unwrap();
        let mut manager2 = EffectManager::from_save(save).unwrap();
        assert_eq!(manager.layers.len(), manager2.layers.len());
        for (pos, l) in manager.layers.iter_mut().enumerate() {
            assert_eq!(l.effect_keys, manager2.layers[pos].effect_keys);
        }
    }
}
