use std::env;
use std::os::unix::net::UnixStream;
mod comms;
mod daemon_core;
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
        "read" => {
            read_param(socket, 0x02);
        }
        "write" => {
            //write_param(socket);
        }
        _ => {
            print_help(format!("Invalid argument: {}", args[1]).as_str());
            std::process::exit(1);
        }
    }
}

fn read_param(socket: UnixStream, param_id: u8) {
    let send: [u8; 2] = [0x00, param_id];
    if let Some(res) = send_data_resp(socket, &send) {
        println!("Result: {:#02X}", res[1]);
    } else {
        eprintln!("Error writing to socket");
    }
}

fn write_param(mut socket: UnixStream, param_id: u8, value: u8) {
    let send: [u8; 3] = [0x01, param_id, value];
    socket.write(&send).unwrap();
}

fn send_data_resp(mut socket: UnixStream, buf: &[u8]) -> Option<[u8; 4096]> {
    let mut buffer: [u8; 4096] = [0x00; 4096];
    if let Ok(_) = socket.write(&buf) {
        socket.read(&mut buffer).unwrap();
        Some(buffer)
    } else {
        None
    }
}
