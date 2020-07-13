use crate::effects;
use serde::{Deserialize, Serialize};
use std::fs;
use std::fs::File;
use std::io;
use std::io::prelude::*;

const SETTINGS_FILE: &str = "/usr/share/razercontrol/daemon.json";
const EFFECTS_FILE: &str = "/usr/share/razercontrol/effects.json";

#[derive(Serialize, Deserialize)]
pub struct Configuration {
    pub power_mode: u8,
    pub fan_rpm: i32,
    pub brightness: u8,
}

impl Configuration {
    pub fn new() -> Configuration {
        return Configuration {
            power_mode: 0,
            fan_rpm: 0,
            brightness: 128,
        };
    }

    pub fn write_to_file(&mut self) -> io::Result<()> {
        let j: String = serde_json::to_string_pretty(&self)?;
        File::create(SETTINGS_FILE)?.write_all(j.as_bytes())?;
        Ok(())
    }

    pub fn read_from_config() -> io::Result<Configuration> {
        let str = fs::read_to_string(SETTINGS_FILE)?;
        let res: Configuration = serde_json::from_str(str.as_str())?;
        Ok(res)
    }

    pub fn save_effects(mut e: serde_json::Value) -> io::Result<()> {
        let j: String = serde_json::to_string_pretty(&e)?;
        File::create(EFFECTS_FILE)?.write_all(j.as_bytes())?;
        Ok(())
    }

    pub fn load_effects() -> Option<effects::EffectManager> {
        if let Ok(str) = fs::read_to_string(EFFECTS_FILE) {
            if let Ok(json) = serde_json::from_str::<serde_json::Value>(str.as_str()) {
                return effects::EffectManager::from_save(json);
            }
        }
        return None;
    }
}
