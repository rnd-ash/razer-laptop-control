extern crate tiny_nix_ipc;


mod rgb;
mod core;
use std::process;
use std::os::unix::net::{UnixStream, UnixListener};
use std::{error::Error, thread};
use signal_hook::{iterator::Signals, SIGTERM, SIGINT};


// Main function for daemon
fn main() {

    // Signal handler - cleanup if we are told to exit
    let signals = Signals::new(&[SIGINT, SIGINT]).unwrap();
    thread::spawn(move || {
        for _ in signals.forever() {
            println!("Received signal, cleaning up");
            std::fs::remove_file(core::SOCKET_PATH).unwrap();
            std::process::exit(0);
        }
    });


    // Setup driver core framework
    let mut core = core::DriverHandler::new().expect("Error. Is kernel module loaded?");
    
    let mut c = core::configuration::new();
    
    if let Ok(_) = std::fs::metadata(core::SOCKET_PATH) {
        eprintln!("UNIX Socket already exists at {}. Is another daemon running?", core::SOCKET_PATH);
        std::process::exit(1);
    }
    if let Ok(listener) = UnixListener::bind(core::SOCKET_PATH) {
        let mut perms = std::fs::metadata(core::SOCKET_PATH).unwrap().permissions();
        perms.set_readonly(false);
        std::fs::set_permissions(core::SOCKET_PATH, perms);
        println!("Listening to events!");
        for stream in listener.incoming() {
            match stream {
                Ok(stream) => {
                    thread::spawn(|| handle_data(stream));
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

fn handle_data(stream: UnixStream) {
    println!("Data!");
}
