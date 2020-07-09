use std::path::Path;
use std::fs;
use std::io;

use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::prelude::*;

// Driver path
const DRIVER_DIR: &'static str = "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver";

// Socket name
pub const SOCKET_PATH: &'static str = "/tmp/razercontrol-socket";

// Module Sysfs file names
pub mod drv_fs_names {
    pub const FAN_RPM: &'static str = "fan_rpm";
    pub const BRIGHTNESS: &'static str = "brightness";
    pub const RGB_MAP: &'static str = "key_colour_map";
    pub const POWER_MODE: &'static str = "power_mode";
}



const SETTINGS_FILE: &str = "/usr/share/razercontrol/daemon.json";



#[derive(Serialize, Deserialize, Debug)]
pub struct configuration {
    pub power_mode: u8,
    pub fan_rpm: i32,
    pub brightness: u8
}

impl configuration {
    pub fn new() -> configuration {
        return configuration {
            power_mode: 0,
            fan_rpm : 0,
            brightness: 128
        }
    }

    pub fn write_to_file(&mut self) -> io::Result<()> {
        let mut j: String = serde_json::to_string(&self)?;
        j += "\n";
        File::create(SETTINGS_FILE)?.write_all(j.as_bytes())?;
        Ok(())
    }

    pub fn read_from_config() -> io::Result<configuration> {
        let str = fs::read_to_string(SETTINGS_FILE)?;
        let res : configuration = serde_json::from_str(str.as_str())?;
        Ok(res)
    }
}

pub struct DriverHandler {
    device_path: String
}

impl DriverHandler {

    pub fn new() -> Option<DriverHandler> {
        if !DriverHandler::is_drv_loaded() {
            return None;
        }
        match  DriverHandler::get_device_dir(){
            Some(x) => return Some(DriverHandler { device_path: x }),
            None => return None
        }
    }

    /// Checks if driver is loaded by checking the sysfs directory
    fn is_drv_loaded() -> bool {
        return Path::new(DRIVER_DIR).exists();
    }

    /// Returns a Path of the device currently using the driver, Err is returned if there is nothing using it
    /// Scans for the first device as all the USB devices using this driver all point to the same physical hardware
    ///  (Razer is strange)
    fn get_device_dir() -> Option<String> {
        return match fs::read_dir(DRIVER_DIR).unwrap()
        .find(|x| x.as_ref().unwrap().file_name().to_str().unwrap().starts_with("000"))
        .unwrap() {
            Ok(p) => Some(String::from(p.path().to_str().unwrap())),
            Err(_) => None
        }
    }

    /// Writes a String to a sysfs file
    fn write_to_sysfs(&mut self, sysfs_name: &str, val_as_str: String) -> bool {
        match fs::write(self.device_path.clone() + "/" + sysfs_name, val_as_str) {
            Ok(_) => true,
            Err(_) => false
        }
    }

    /// Writes a byte array to a sysfs file
    fn write_to_sysfs_raw(&mut self, sysfs_name: &str, val: Vec<u8>) -> bool {
        match fs::write(self.device_path.clone() + "/" + sysfs_name, val) {
            Ok(_) => true,
            Err(_) => false
        }
    }

    /// Reads a String from sysfs file (Removing the \n)
    fn read_from_sysfs(&mut self, sysfs_name: &str) -> Option<String> {
        match fs::read_to_string(self.device_path.clone() + "/" + sysfs_name) {
            Ok(s) => Some(s.clone().trim_end_matches('\n').to_string()),
            Err(_) => None
        }
    }




    // RGB Map is write only
    pub fn write_rgb_map(&mut self, map: Vec<u8>) -> bool {
        return self.write_to_sysfs_raw(drv_fs_names::BRIGHTNESS, map);
    }



    // Brightness is read + write
    pub fn write_brightness(&mut self, lvl: u8) -> bool {
        return self.write_to_sysfs(drv_fs_names::BRIGHTNESS, String::from(format!("{}", lvl)))
    }
    pub fn read_brightness(&mut self) -> u8 {
        return match self.read_from_sysfs(drv_fs_names::BRIGHTNESS) {
            Some(x) => x.parse::<u8>().unwrap(),
            None => 0
        }
    }



    // Power mode is read + write
    pub fn write_power(&mut self, mode: u8) -> bool {
        return self.write_to_sysfs(drv_fs_names::POWER_MODE, String::from(format!("{}", mode)));
    }

    pub fn read_power(&mut self) -> u8 {
        return match self.read_from_sysfs(drv_fs_names::POWER_MODE) {
            Some(x) => x.parse::<u8>().unwrap(),
            None => 0
        }
    }



    // Fan RPM is read + write
    pub fn write_fan_rpm(&mut self, rpm: i32) -> bool {
        return self.write_to_sysfs(drv_fs_names::FAN_RPM, String::from(format!("{}", rpm)))
    }
    pub fn read_fan_rpm(&mut self) -> i32 {
        match self.read_from_sysfs(drv_fs_names::FAN_RPM) {
            Some(x) => {
                if x == "Auto" {
                    return 0
                } else {
                    return x.split(" ")
                            .map(|s| s.to_string())
                            .collect::<Vec<String>>()[0]
                            .parse::<i32>().unwrap();
                }
            },
            None => return -1
        }
    }
}



