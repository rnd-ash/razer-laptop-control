use serde::{Deserialize, Serialize};
use std::io::{Read, Write};
use std::os::unix::net::{UnixListener, UnixStream};

/// Razer laptop control socket path
pub const SOCKET_PATH: &'static str = "/tmp/razercontrol-socket";

#[derive(Serialize, Deserialize, Debug)]
/// Represents data sent TO the daemon
pub enum DaemonCommand {
    SetFanSpeed { rpm: i32 },      // Fan speed
    GetFanSpeed(),                 // Get (Fan speed)
    SetPowerMode { pwr: u8 },      // Power mode
    GetPwrLevel(),                 // Get (Power mode)
    GetKeyboardRGB { layer: i32 }, // Layer ID
    GetCfg(),                      // Request curr settings for fan + power
}

#[derive(Serialize, Deserialize, Debug)]
/// Represents data sent back from Daemon after it receives
/// a command.
pub enum DaemonResponse {
    SetFanSpeed { result: bool },                    // Response
    GetFanSpeed { rpm: i32 },                        // Get (Fan speed)
    SetPowerMode { result: bool },                   // Response
    GetPwrLevel { pwr: u8 },                         // Get (Power mode)
    GetKeyboardRGB { layer: i32, rgbdata: Vec<u8> }, // Response (RGB) of 90 keys
    GetCfg { fan_rpm: i32, pwr: u8 },                // Fan speed, power mode
}

pub fn bind() -> Option<UnixStream> {
    if let Ok(socket) = UnixStream::connect(SOCKET_PATH) {
        return Some(socket);
    } else {
        return None;
    }
}

pub fn create() -> Option<UnixListener> {
    if let Ok(_) = std::fs::metadata(SOCKET_PATH) {
        eprintln!("UNIX Socket already exists. Is another daemon running?");
        return None;
    }
    if let Ok(listener) = UnixListener::bind(SOCKET_PATH) {
        let mut perms = std::fs::metadata(SOCKET_PATH).unwrap().permissions();
        perms.set_readonly(false);
        if std::fs::set_permissions(SOCKET_PATH, perms).is_err() {
            eprintln!("Could not set socket permissions");
            return None;
        }
        return Some(listener);
    }
    return None;
}

pub fn send_to_daemon(command: DaemonCommand, mut sock: UnixStream) -> Option<DaemonResponse> {
    if let Ok(encoded) = bincode::serialize(&command) {
        if sock.write_all(&encoded).is_ok() {
            let mut buf = [0 as u8; 4096];
            return match sock.read(&mut buf) {
                Ok(_) => read_from_socked_resp(&buf),
                Err(_) => {
                    eprintln!("Read failed!");
                    None
                }
            };
        } else {
            eprintln!("Socket write failed!");
        }
    }
    return None;
}

/// Deserializes incomming bytes in order to return
/// a `DaemonResponse`. None is returned if deserializing failed
fn read_from_socked_resp(bytes: &[u8]) -> Option<DaemonResponse> {
    match bincode::deserialize::<DaemonResponse>(bytes) {
        Ok(res) => {
            println!("RES: {:?}", res);
            return Some(res);
        }
        Err(e) => {
            println!("RES ERROR: {}", e);
            return None;
        }
    }
}

/// Deserializes incomming bytes in order to return
/// a `DaemonCommand`. None is returned if deserializing failed
pub fn read_from_socket_req(bytes: &[u8]) -> Option<DaemonCommand> {
    match bincode::deserialize::<DaemonCommand>(bytes) {
        Ok(res) => {
            println!("REQ: {:?}", res);
            return Some(res);
        }
        Err(e) => {
            println!("REQ ERROR: {}", e);
            return None;
        }
    }
}
