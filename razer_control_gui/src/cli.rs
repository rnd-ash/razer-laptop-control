use std::env;
use std::os::unix::net::UnixStream;
mod comms;
use std::io::{Read, Write};
use std::time::{SystemTime, UNIX_EPOCH};

fn print_help(reason: &str) {
    if reason.len() > 1 {
        println!("ERROR: {}", reason);
    }
    println!("Help:");
    println!("./razer-cli read <attr>");
    println!("./razer-cli write <attr>");
    println!("");
    println!("Where 'attr':");
    println!("- fan_rpm -> Cooling fan RPM. 0 is automatic");
    println!("- power   -> Power mode.");
    println!("              0 = Normal");
    println!("              1 = Gaming");
    println!("              2 = Balanced");
}

fn main() {
    if let Some(socket) = comms::bind() {
        
    } else {
        eprintln!("Unable to bind to socket. Is daemon running?");
    }
}
