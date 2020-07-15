extern crate tiny_nix_ipc;

mod comms;
mod config;
mod driver_sysfs;
mod kbd;
use crate::kbd::Effect;
use signal_hook::{iterator::Signals, SIGINT, SIGTERM};
use std::io::Read;
use std::os::unix::net::{UnixListener, UnixStream};
use std::thread;

// Main function for daemon
fn main() {
    // Setup driver core frameworks

    let mut manager = kbd::EffectManager::new();
    let e1 = kbd::effects::WaveGradient::new(vec![255, 0, 0, 0, 0, 255, 0]);
    let e2 = kbd::effects::BreathSingle::new(vec![255, 255, 0, 10]);
    let mut mask : [bool; 90] = [
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    ];
    manager.push_effect(e1, mask);

    mask = [
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
    ];
    manager.push_effect(e2, mask);

    std::thread::spawn(move || loop {
        manager.update();
        std::thread::sleep(std::time::Duration::from_millis(33));
    });


    if driver_sysfs::get_path().is_none() {
        eprintln!("Error. Kernel module not found!");
        std::process::exit(1);
    }

    let mut cfg: config::Configuration;
    if let Ok(config) = config::Configuration::read_from_config() {
        cfg = config;
        driver_sysfs::write_brightness(cfg.brightness);
        driver_sysfs::write_fan_rpm(cfg.fan_rpm);
        driver_sysfs::write_power(cfg.power_mode);
    } else {
        println!("No configuration file found, creating a new one");
        cfg = config::Configuration::new();
    }

    // Signal handler - cleanup if we are told to exit
    let signals = Signals::new(&[SIGINT, SIGTERM]).unwrap();
    let clean_thread = thread::spawn(move || {
        for _ in signals.forever() {
            println!("Received signal, cleaning up");
            cfg.write_to_file().unwrap();
            if std::fs::metadata(comms::SOCKET_PATH).is_ok() {
                std::fs::remove_file(comms::SOCKET_PATH).unwrap();
                if let Err(_) = cfg.write_to_file() {
                    eprintln!("Error saving configuration!");
                }
            }
            std::process::exit(0);
        }
    });

    if let Ok(_) = std::fs::metadata(comms::SOCKET_PATH) {
        eprintln!(
            "UNIX Socket already exists at {}. Is another daemon running?",
            comms::SOCKET_PATH
        );
        std::process::exit(1);
    }
    if let Ok(listener) = UnixListener::bind(comms::SOCKET_PATH) {
        let mut perms = std::fs::metadata(comms::SOCKET_PATH).unwrap().permissions();
        perms.set_readonly(false);
        if std::fs::set_permissions(comms::SOCKET_PATH, perms).is_err() {
            eprintln!("Could not set socket permissions");
            std::process::exit(1);
        }
        println!("Listening to events!");
        for stream in listener.incoming() {
            match stream {
                Ok(stream) => {
                    handle_data(stream);
                }
                Err(_) => {
                    eprintln!("WARN: Broken stream, ignored");
                    break;
                }
            }
        }
    } else {
        eprintln!("Could not create Unix socket!");
        std::process::exit(1);
    }
    clean_thread.join().unwrap();
}

fn handle_data(mut stream: UnixStream) {
    let mut buffer = [0 as u8; 4096];
    match stream.read(&mut buffer) {
        Ok(size) => println!("Found {} bytes", size),
        Err(_) => {}
    }
}
