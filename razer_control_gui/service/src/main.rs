extern crate tiny_nix_ipc;

mod comms;
mod config;
mod daemon_core;
mod effects;
mod rgb;
use signal_hook::{iterator::Signals, SIGINT, SIGTERM};
use std::io::{Read, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::{error::Error, sync, thread};

// Main function for daemon
fn main() {
    // Setup driver core frameworks
    let mut drv_core = daemon_core::DriverHandler::new().expect("Error. Is kernel module loaded?");
    let mut cfg: config::Configuration;
    if let Ok(config) = config::Configuration::read_from_config() {
        cfg = config;
        drv_core.write_brightness(cfg.brightness);
        drv_core.write_fan_rpm(cfg.fan_rpm);
        drv_core.write_power(cfg.power_mode);
    } else {
        println!("No configuration file found, creating a new one");
        cfg = config::Configuration::new();
    }

    let mut effects: effects::EffectManager;
    if let Some(e) = config::Configuration::load_effects() {
        effects = e;
    } else {
        println!("No effects file found, creating a new one");
        effects = effects::EffectManager::new();
        // Add a new layer (Static Green)
        effects.push_effect(Box::new(effects::StaticEffect::new(0, 255, 0)), &[true; 90]);
    }

    thread::spawn(move || loop {
        effects.update(&mut drv_core);
    });

    // Signal handler - cleanup if we are told to exit
    let signals = Signals::new(&[SIGINT, SIGINT]).unwrap();
    thread::spawn(move || {
        for _ in signals.forever() {
            println!("Received signal, cleaning up");
            cfg.write_to_file().unwrap();
            if let Some(s) = effects.clone().get_save() {
                config::Configuration::save_effects(s);
            }
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
                    handle_data(stream, &mut drv_core);
                }
                Err(err) => {
                    eprintln!("WARN: Broken stream, ignored");
                    break;
                }
            }
        }
    } else {
        eprintln!("Could not create Unix socket!");
        std::process::exit(1);
    }
}

fn handle_data(mut stream: UnixStream, handler: &mut daemon_core::DriverHandler) {
    let mut buffer = [0 as u8; 4096];
    match stream.read(&mut buffer) {
        Ok(size) => handler.process_payload(&buffer, stream),
        Err(_) => {}
    }
}
