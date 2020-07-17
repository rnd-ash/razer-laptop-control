use std::env;
mod comms;

fn print_help(reason: &str) -> ! {
    let mut ret_code = 0;
    if reason.len() > 1 {
        println!("ERROR: {}", reason);
        ret_code = 1;
    }
    println!("Help:");
    println!("./razer-cli read <attr>");
    println!("./razer-cli write <attr>");
    println!("./razer-cli write colour red green blue");
    println!("");
    println!("Where 'attr':");
    println!("- fan -> Cooling fan RPM. 0 is automatic");
    println!("- power   -> Power mode.");
    println!("              0 = Normal");
    println!("              1 = Gaming");
    println!("              2 = Balanced");
    println!("");
    std::process::exit(ret_code);
}

fn main() {
    // Check if socket is OK
    if std::fs::metadata(comms::SOCKET_PATH).is_err() {
        eprintln!("Error. Socket doesn't exit. Is daemon running?");
        std::process::exit(1);
    }
    let args : Vec<_> = env::args().collect();
    if args.len() < 3 {
        print_help("Not enough args supplied");
    }
    match args[1].to_ascii_lowercase().as_str() {
        "read" => {
            match args[2].to_ascii_lowercase().as_str() {
                "fan" => read_fan_rpm(),
                "power" => read_power_mode(),
                _ => print_help(format!("Unrecognised option to read: `{}`", args[2]).as_str())
            }
        },
        "write" => {
            // Special case for setting kbd colour
            if args[2].to_ascii_lowercase().as_str() == "colour" {
                if args.len() != 6 {
                    print_help("Invalid number of args. Colour requires 3 params for r,g,b");
                }
                let r = args[3].parse::<u8>();
                let g = args[4].parse::<u8>();
                let b = args[5].parse::<u8>();
                if r.is_err() || g.is_err() || b.is_err() {
                    print_help("Invalid number detected. r,g,b must be between 0 and 255!")
                }
                write_colour(r.unwrap(), g.unwrap(), b.unwrap());
                return;
            }
            if args.len() != 4 {
                print_help("Invalid number of args supplied");
            }
            if let Ok(processed) = args[3].parse::<i32>() {
                match args[2].to_ascii_lowercase().as_str() {
                    "fan" => write_fan_speed(processed),
                    "power" => write_pwr_mode(processed),
                    _ => print_help(format!("Unrecognised option to read: `{}`", args[2]).as_str())
                }
            } else {
                print_help(format!("`{}` is not a valid number", args[3]).as_str())
            }
        },
        _ => print_help(format!("Unrecognised argument: `{}`", args[1]).as_str())
    }
}


fn send_data(opt: comms::DaemonCommand) -> Option<comms::DaemonResponse> {
    if let Some(socket) = comms::bind() {
        return comms::send_to_daemon(opt, socket);
    } else {
        eprintln!("Error. Cannot bind to socket");
        return None;
    }
}

fn read_fan_rpm() {
    if let Some(resp) = send_data(comms::DaemonCommand::GetFanSpeed()) {
        if let comms::DaemonResponse::GetFanSpeed {rpm } = resp {
            let rpm_desc : String = match rpm {
                f if f < 0 => String::from("Unknown"),
                0 => String::from("Auto (0)"),
                _ => format!("{} RPM", rpm)
            };
            println!("Current fan setting: {}", rpm_desc);
        } else {
            eprintln!("Daemon responded with invalid data!");
        }
    }
}

fn read_power_mode() {
    if let Some(resp) = send_data(comms::DaemonCommand::GetPwrLevel()) {
        if let comms::DaemonResponse::GetPwrLevel {pwr } = resp {
            let power_desc : &str = match pwr {
                0 => "Balanced",
                1 => "Gaming",
                2 => "Creator",
                _ => "Unknown"
            };
            println!("Current power setting: {}", power_desc);
        } else {
            eprintln!("Daemon responded with invalid data!");
        }
    }
}

fn write_pwr_mode(x: i32) {
    if !(x >= 0 && x <= 2) {
        print_help("Power mode must be 0, 1 or 2");
    }
    if let Some(_) = send_data(comms::DaemonCommand::SetPowerMode { pwr: x as u8 }) {
        read_power_mode()
    } else {
        eprintln!("Unknown error!");
    }
}

fn write_fan_speed(x: i32) {
    if let Some(_) = send_data(comms::DaemonCommand::SetFanSpeed { rpm: x }) {
        read_fan_rpm()
    } else {
        eprintln!("Unknown error!");
    }
}

fn write_colour(r: u8, g: u8, b: u8) {
    if let Some(_) = send_data(comms::DaemonCommand::SetColour { r, g, b }) {
        read_fan_rpm()
    } else {
        eprintln!("Unknown error!");
    }
}