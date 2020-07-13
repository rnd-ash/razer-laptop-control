use std::fs;
use std::io::prelude::*;
use std::os::unix::net::UnixStream;
use std::path::Path;

// Driver path
const DRIVER_DIR: &'static str =
    "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver";

#[derive(Clone, Debug)]
pub struct DriverHandler {
    device_path: String,
}

impl DriverHandler {
    pub fn new() -> Option<DriverHandler> {
        if !DriverHandler::is_drv_loaded() {
            return None;
        }
        match DriverHandler::get_device_dir() {
            Some(x) => return Some(DriverHandler { device_path: x }),
            None => return None,
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
        return match fs::read_dir(DRIVER_DIR)
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
        };
    }

    /// Writes a String to a sysfs file
    fn write_to_sysfs(&mut self, sysfs_name: &str, val_as_str: String) -> bool {
        match fs::write(self.device_path.clone() + "/" + sysfs_name, val_as_str) {
            Ok(_) => true,
            Err(_) => false,
        }
    }

    /// Writes a byte array to a sysfs file
    fn write_to_sysfs_raw(&mut self, sysfs_name: &str, val: Vec<u8>) -> bool {
        match fs::write(self.device_path.clone() + "/" + sysfs_name, val) {
            Ok(_) => true,
            Err(_) => false,
        }
    }

    /// Reads a String from sysfs file (Removing the \n)
    fn read_from_sysfs(&mut self, sysfs_name: &str) -> Option<String> {
        match fs::read_to_string(self.device_path.clone() + "/" + sysfs_name) {
            Ok(s) => Some(s.clone().trim_end_matches('\n').to_string()),
            Err(_) => None,
        }
    }

    // RGB Map is write only
    pub fn write_rgb_map(&mut self, map: Vec<u8>) -> bool {
        return self.write_to_sysfs_raw("key_colour_map", map);
    }

    // Brightness is read + write
    pub fn write_brightness(&mut self, lvl: u8) -> bool {
        return self.write_to_sysfs("brightness", String::from(format!("{}", lvl)));
    }
    pub fn read_brightness(&mut self) -> u8 {
        return match self.read_from_sysfs("brightness") {
            Some(x) => x.parse::<u8>().unwrap(),
            None => 0,
        };
    }

    // Power mode is read + write
    pub fn write_power(&mut self, mode: u8) -> bool {
        return self.write_to_sysfs("power_mode", String::from(format!("{}", mode)));
    }

    pub fn read_power(&mut self) -> u8 {
        return match self.read_from_sysfs("power_mode") {
            Some(x) => x.parse::<u8>().unwrap(),
            None => 0,
        };
    }

    // Fan RPM is read + write
    pub fn write_fan_rpm(&mut self, rpm: i32) -> bool {
        return self.write_to_sysfs("fan_rpm", String::from(format!("{}", rpm)));
    }

    pub fn read_fan_rpm(&mut self) -> i32 {
        match self.read_from_sysfs("fan_rpm") {
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

    pub fn process_payload(&mut self, buffer: &[u8], socket: UnixStream) {
        match buffer[0] {
            0 => {
                // Read
                match self.read_param(buffer) {
                    (true, x) => self.respond_ok(socket, &x), // 0x01 is write succeeded
                    (false, _) => self.respond_err(socket),
                }
            }
            1 => {
                // Write
                match self.write_param(buffer) {
                    true => self.respond_ok(socket, &[0x01 as u8]), // 0x01 is write succeeded
                    false => self.respond_err(socket),
                }
            }
            _ => {
                eprintln!("Unknown CMD ID: {:#02X}", buffer[0]);
                self.respond_err(socket);
            }
        }
    }

    pub fn read_param(&mut self, args: &[u8]) -> (bool, Vec<u8>) {
        println!("{:?}", args);
        match args[2] {
            0x00 => {
                println!("Reading fan RPM");
                let rpm = self.read_fan_rpm();
                if rpm != 0 {
                    return (true, vec![(rpm as f32 / 100.0) as u8]);
                } else {
                    return (true, vec![0 as u8]);
                }
            }
            0x01 => {
                println!("Reading power");
                return (true, vec![self.read_power() as u8]);
            }
            _ => {
                eprintln!("Unknown write param {:#02X}", args[2]);
                (false, vec![])
            }
        }
    }

    pub fn write_param(&mut self, args: &[u8]) -> bool {
        match args[2] {
            0x00 => return self.write_fan_rpm(args[3] as i32 * 100),
            0x01 => return self.write_power(args[3]),
            _ => {
                eprintln!("Unknown write param {:#02X}", args[2]);
                false
            }
        }
    }

    pub fn respond_err(&mut self, socket: UnixStream) {
        let res: [u8; 2] = [0xFF, 0x00]; // 0xFF is error
        self.send_payload_resp(socket, &res);
    }

    pub fn respond_ok(&mut self, socket: UnixStream, args: &[u8]) {
        let mut resp: Vec<u8> = Vec::with_capacity(args.len() + 1);
        resp.push(0x00); // Ok!
        resp.extend(args);
        self.send_payload_resp(socket, &resp);
    }

    pub fn send_payload_resp(&mut self, mut socket: UnixStream, res: &[u8]) {
        if socket.write(res).is_err() {
            eprintln!("Error sending response back via socket");
        }
    }
}
