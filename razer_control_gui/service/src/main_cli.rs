use std::env;
use std::os::unix::net::UnixStream;
mod comms;
use std::io::{Read, Write};

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
    let socket: UnixStream = match UnixStream::connect(comms::SOCKET_PATH) {
        Ok(sock) => {
            println!("Socket connected!");
            sock
        }
        Err(_) => {
            eprintln!("Couldn't connect to razer socket - Is daemon running?");
            std::process::exit(1);
        }
    };

    let args: Vec<String> = env::args().collect();

    if args.len() < 3 {
        print_help("Not enough args supplied");
        std::process::exit(1);
    }

    match args[1].as_str() {
        "read" => {}
        "write" => {
            //write_param(socket);
        }
        _ => {
            print_help(format!("Invalid argument: {}", args[1]).as_str());
            std::process::exit(1);
        }
    }
}
