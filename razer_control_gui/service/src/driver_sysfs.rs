use lazy_static::lazy_static;

use std::fs;

// Driver path
const DRIVER_DIR: &'static str =
    "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver";

lazy_static! {
    static ref SYSFS_PATH: Option<String> = {
        match fs::read_dir(DRIVER_DIR)
            .unwrap()
            .find(|x| {
                x.as_ref()
                    .unwrap()
                    .file_name()
                    .to_str()
                    .unwrap()
                    .starts_with("000")
            })
            .unwrap()
        {
            Ok(p) => Some(String::from(p.path().to_str().unwrap())),
            Err(_) => None,
        }
    };
}

pub fn get_path() -> Option<String> {
    SYSFS_PATH.clone()
}

/// Writes a String to a sysfs file
fn write_to_sysfs(sysfs_name: &str, val_as_str: String) -> bool {
    match fs::write(SYSFS_PATH.clone().unwrap() + "/" + sysfs_name, val_as_str) {
        Ok(_) => true,
        Err(_) => false,
    }
}

/// Writes a byte array to a sysfs file
fn write_to_sysfs_raw(sysfs_name: &str, val: Vec<u8>) -> bool {
    match fs::write(SYSFS_PATH.clone().unwrap() + "/" + sysfs_name, val) {
        Ok(_) => true,
        Err(_) => false,
    }
}

/// Reads a String from sysfs file (Removing the \n)
fn read_from_sysfs(sysfs_name: &str) -> Option<String> {
    match fs::read_to_string(SYSFS_PATH.clone().unwrap() + "/" + sysfs_name) {
        Ok(s) => Some(s.clone().trim_end_matches('\n').to_string()),
        Err(_) => None,
    }
}

// RGB Map is write only
pub fn write_rgb_map(map: Vec<u8>) -> bool {
    return write_to_sysfs_raw("key_colour_map", map);
}

// Brightness is read + write
pub fn write_brightness(lvl: u8) -> bool {
    return write_to_sysfs("brightness", String::from(format!("{}", lvl)));
}
pub fn read_brightness() -> u8 {
    return match read_from_sysfs("brightness") {
        Some(x) => x.parse::<u8>().unwrap(),
        None => 0,
    };
}

// Power mode is read + write
pub fn write_power(mode: u8) -> bool {
    return write_to_sysfs("power_mode", String::from(format!("{}", mode)));
}

pub fn read_power() -> u8 {
    return match read_from_sysfs("power_mode") {
        Some(x) => x.parse::<u8>().unwrap(),
        None => 0,
    };
}

// Fan RPM is read + write
pub fn write_fan_rpm(rpm: i32) -> bool {
    return write_to_sysfs("fan_rpm", String::from(format!("{}", rpm)));
}

pub fn read_fan_rpm() -> i32 {
    match read_from_sysfs("fan_rpm") {
        Some(x) => {
            if x == "Auto" {
                return 0;
            } else {
                return x.split(" ").map(|s| s.to_string()).collect::<Vec<String>>()[0]
                    .parse::<i32>()
                    .unwrap();
            }
        }
        None => return -1,
    }
}
