mod comms;
mod config;
mod driver_sysfs;
mod kbd;
use crate::kbd::Effect;
use lazy_static::lazy_static;
use signal_hook::{iterator::Signals, SIGINT, SIGTERM};
use std::io::prelude::*;
use std::io::{Read, Write};
use std::os::unix::net::UnixStream;
use std::sync::Mutex;
use std::{thread, time};

lazy_static! {
    static ref EFFECT_MANAGER: Mutex<kbd::EffectManager> = Mutex::new(kbd::EffectManager::new());
    static ref CONFIG: Mutex<config::Configuration> = {
        match config::Configuration::read_from_config() {
            Ok(c) => Mutex::new(c),
            Err(_) => Mutex::new(config::Configuration::new()),
        }
    };
}

fn push_effect(effect: Box<dyn Effect>, mask: [bool; 90]) {
    EFFECT_MANAGER.lock().unwrap().push_effect(effect, mask)
}

// Main function for daemon
fn main() {
    let mut tries = 0;
    // Wait for our sysfs to be read - try for 30 seconds
    while std::fs::metadata(driver_sysfs::DRIVER_DIR).is_err() {
        println!("Waiting for sysfs to be ready");
        thread::sleep(time::Duration::from_millis(1000));
        tries += 1;
        if tries == 30 {
            eprintln!("Timed out waiting for sysfs after a minute!");
            std::process::exit(1);
        }
    }
    println!("Sysfs ready! Starting daemon");

    // Start the keyboard animator thread,
    // This thread also periodically checks the machine power
    std::thread::spawn(move || {
        let mut last_psu_status : driver_sysfs::PowerSupply = driver_sysfs::PowerSupply::UNK;
        loop {
            EFFECT_MANAGER.lock().unwrap().update();
            std::thread::sleep(std::time::Duration::from_millis(kbd::ANIMATION_SLEEP_MS));
            let new_psu = driver_sysfs::read_power_source();
            if last_psu_status != new_psu {
                println!("Power source changed! Now {:?}", new_psu);
            }
            last_psu_status = new_psu;
        }
    });

    if driver_sysfs::get_path().is_none() {
        eprintln!("Error. Kernel module not found!");
        std::process::exit(1);
    }

    if let Ok(c) = CONFIG.lock() {
        driver_sysfs::write_brightness(c.brightness);
        driver_sysfs::write_fan_rpm(c.fan_rpm);
        driver_sysfs::write_power(c.power_mode);
        driver_sysfs::write_cpu_boost(c.cpu_boost);
        driver_sysfs::write_gpu_boost(c.gpu_boost);
        if let Ok(json) = config::Configuration::read_effects_file() {
            EFFECT_MANAGER.lock().unwrap().load_from_save(json);
        } else {
            println!("No effects save, creating a new one");
            // No effects found, start with a green static layer, just like synapse
            EFFECT_MANAGER.lock().unwrap().push_effect(
                kbd::effects::Static::new(vec![0, 255, 0]), 
                [true; 90]
            );
        }
    }

    // Signal handler - cleanup if we are told to exit
    let signals = Signals::new(&[SIGINT, SIGTERM]).unwrap();
    let clean_thread = thread::spawn(move || {
        for _ in signals.forever() {
            println!("Received signal, cleaning up");
            if let Ok(mut c) = CONFIG.lock() {
                c.write_to_file().unwrap();
            }
            let json = EFFECT_MANAGER.lock().unwrap().save();
            config::Configuration::write_effects_save(json).unwrap();
            if std::fs::metadata(comms::SOCKET_PATH).is_ok() {
                std::fs::remove_file(comms::SOCKET_PATH).unwrap();
            }
            std::process::exit(0);
        }
    });

    if let Some(listener) = comms::create() {
        for stream in listener.incoming() {
            match stream {
                Ok(stream) => handle_data(stream),
                Err(_) => {} // Don't care about this
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
    if stream.read(&mut buffer).is_err() {
        return;
    }

    if let Some(cmd) = comms::read_from_socket_req(&buffer) {
        if let Some(s) = process_client_request(cmd) {
            if let Ok(x) = bincode::serialize(&s) {
                stream.write_all(&x).unwrap();
            }
        }
    }
}

pub fn process_client_request(cmd: comms::DaemonCommand) -> Option<comms::DaemonResponse> {
    return match cmd {
        comms::DaemonCommand::GetCfg() => {
            let fan_rpm = CONFIG.lock().unwrap().fan_rpm;
            let pwr = CONFIG.lock().unwrap().power_mode;
            Some(comms::DaemonResponse::GetCfg { fan_rpm, pwr })
        }
        comms::DaemonCommand::SetPowerMode { pwr, cpu, gpu } => {
            let mut res = false;
            if let  Ok(mut x) = CONFIG.lock() {
                x.power_mode = pwr;
                x.cpu_boost = cpu;
                x.gpu_boost = gpu;
                x.write_to_file().unwrap();
            }

            if driver_sysfs::write_power(pwr) {
                if driver_sysfs::write_cpu_boost(cpu) {
                    if driver_sysfs::write_gpu_boost(gpu) {
                        res = true;
                    }
                }
            }
            Some(comms::DaemonResponse::SetPowerMode { result: res })
        },
        comms::DaemonCommand::SetFanSpeed { rpm } => {
            if let  Ok(mut x) = CONFIG.lock() {
                x.fan_rpm = rpm;
                x.write_to_file().unwrap();
            }
            Some(comms::DaemonResponse::SetFanSpeed { result: driver_sysfs::write_fan_rpm(rpm) })
        },
        comms::DaemonCommand::GetKeyboardRGB { layer } => {
            let map = EFFECT_MANAGER.lock().unwrap().get_map(layer);
            Some(comms::DaemonResponse::GetKeyboardRGB {
                layer,
                rgbdata: map,
            })
        }
        comms::DaemonCommand::GetFanSpeed() => Some(comms::DaemonResponse::GetFanSpeed { rpm: driver_sysfs::read_fan_rpm() }),
        comms::DaemonCommand::GetPwrLevel() => Some(comms::DaemonResponse::GetPwrLevel { pwr: driver_sysfs::read_power() }),
        comms::DaemonCommand::GetCPUBoost() => Some(comms::DaemonResponse::GetCPUBoost { cpu: driver_sysfs::read_cpu_boost() }),
        comms::DaemonCommand::GetGPUBoost() => Some(comms::DaemonResponse::GetGPUBoost { gpu: driver_sysfs::read_gpu_boost() }),
        comms::DaemonCommand::SetEffect{ name, params } => {
            let mut res = false;
            if let Ok(mut k) = EFFECT_MANAGER.lock() {
                res = true;
                let effect = match name.as_str() {
                    "static" => Some(kbd::effects::Static::new(params)),
                    "static_gradient" => Some(kbd::effects::StaticGradient::new(params)),
                    "wave_gradient" => Some(kbd::effects::WaveGradient::new(params)),
                    "breathing_single" => Some(kbd::effects::BreathSingle::new(params)),
                    _ => None
                };

                if let Some(e) = effect {
                    k.pop_effect(); // Remove old layer
                    k.push_effect(
                        e,
                        [true; 90]
                    );
                } else {
                    res = false
                }
            }
            Some(comms::DaemonResponse::SetEffect{result: res})
        }

        _ => {
            eprintln!("Error. Unrecognised request!");
            None
        }
    };
}
