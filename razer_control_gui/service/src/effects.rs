use crate::rgb;
use crate::core;
use std::time::{SystemTime, UNIX_EPOCH};

const ANIMATIONS_DELAY_MS : u128 = 33; // 33 ms ~= 30fps

pub struct EffectManager {
    layerHistory: Vec<[u8; 90]>,
    effects: Vec<Box<dyn Effect>>,
    lastUpdateTime: u128,
    combined: rgb::KeyboardData, // Actual rendered keyboard
}

impl EffectManager {
    fn get_millis(&mut self) -> u128 {
        SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis()
    }

    pub fn new() -> EffectManager {
        EffectManager {
            layerHistory: vec![],
            effects: vec![],
            lastUpdateTime: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis(),
            combined: rgb::KeyboardData::new()
        }
    }

    pub fn update(&mut self, handler: &mut core::DriverHandler) {
        if self.get_millis() - self.lastUpdateTime >= 0 {
            if self.layerHistory.len() == 0 { return } // Return if we have no effects!
            // Update all our effects
            
            // Create a temp map of keyboard
            let mut keyboards : Vec<rgb::KeyboardData> = self.effects.iter_mut().map(|x| x.update()).collect();

            for (key_index, layer_index) in self.layerHistory.last().unwrap().iter().enumerate() {
                self.combined.set_key_at(key_index, keyboards[*layer_index as usize].get_key_at(key_index))
            }


            self.combined.update_kbd(handler); // Render keyboard
            self.lastUpdateTime = self.get_millis();
        }
    }

    pub fn get_effect_layer_count(&mut self) -> usize {
        self.effects.len()
    }

    pub fn push_effect(&mut self, newEffect: Box<dyn Effect>, enabled_keys: &[bool; 90]) {
        self.effects.push(newEffect);
        if self.layerHistory.len() == 0 { // No previous effects stored?
            self.layerHistory.push([0; 90]); // Push empty array of all keys
        } else { // Existing effect found. Merge layers
            let new_layer_id = (self.effects.len()-1) as usize;
            self.layerHistory.push(self.layerHistory.last().unwrap().clone()); // Create a copy of the previous history
            for x in 0..90 { // Iterate over all keys
                if enabled_keys[x] == true { // Found a new key that uses the new layer
                    self.layerHistory[new_layer_id][x] = new_layer_id as u8; // Set the key to use the top-most layer
                }
            }
        }
    }

    pub fn pop_effect(&mut self) {
        self.effects.pop();
        self.layerHistory.pop();
    }
}

pub enum EffectDir {
    Vertical,
    Horizontal,
    Diagonal,
    Circular
}

pub trait Effect {
    fn update(&mut self) -> rgb::KeyboardData;
}

// -- Static effect code --

pub struct StaticEffect {
    pub kbd: rgb::KeyboardData
}

impl StaticEffect {
    pub fn new(red: u8, green: u8, blue: u8) -> StaticEffect {
        let mut k = rgb::KeyboardData::new();
        k.set_kbd_colour(red, green, blue);
        StaticEffect {
            kbd: k
        }
    }
}

impl Effect for StaticEffect {
    fn update(&mut self) -> rgb::KeyboardData {
        // Does nothing on static effect
        return self.kbd;
    }
}

// -- 'Blend' effect code --
pub struct BlendEffect {
    pub kbd: rgb::KeyboardData
}

impl BlendEffect {
    pub fn new(r1: u8, g1: u8, b1: u8, r2: u8, g2: u8, b2: u8, dir: EffectDir) -> BlendEffect {
        let mut k = rgb::KeyboardData::new();
        let dr : f32 = r2 as f32 - r1 as f32; // Delta red
        let dg : f32 = g2 as f32 - g1 as f32; // Delta green
        let db : f32 = b2 as f32 - b1 as f32; // Delta blue
        match dir {
            EffectDir::Vertical => {
                for x in 0..6 {
                    let col_blend_ratio = (x+1) as f32 / 6.0;
                    k.set_row_colour(x, 
                        (r1 as f32 + (dr * col_blend_ratio)) as u8,
                        (g1 as f32 + (dg * col_blend_ratio)) as u8,
                        (b1 as f32 + (db * col_blend_ratio)) as u8);
    
                }
            },
            EffectDir::Horizontal => {
                for x in 0..15 {
                    let col_blend_ratio = (x+1) as f32 / 15.0;
                    k.set_col_colour(x, 
                        (r1 as f32 + (dr * col_blend_ratio)) as u8,
                        (g1 as f32 + (dg * col_blend_ratio)) as u8,
                        (b1 as f32 + (db * col_blend_ratio)) as u8);
    
                }
            },
            _ => { 
                // Unsupported direction, default to vertical
                eprintln!("BlendMode Diagonal unsupported, using vertical");
                return BlendEffect::new(r1, g1, b1, r2, g2, b2, EffectDir::Vertical) 
            }
        }
        BlendEffect {
            kbd : k
        }
    }
}

impl Effect for BlendEffect {
    fn update(&mut self) -> rgb::KeyboardData {
        // Does nothing on static effect
        return self.kbd;
    }
}

