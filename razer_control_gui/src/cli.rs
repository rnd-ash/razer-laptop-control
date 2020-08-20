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
    println!("./razer-cli write effect <effect name> <params>");
    println!("");
    println!("Where 'attr':");
    println!("- fan -> Cooling fan RPM. 0 is automatic");
    println!("- power   -> Power mode.");
    println!("              0 = Normal");
    println!("              1 = Gaming");
    println!("              2 = Balanced");
    println!("");
    println!("- effect:");
    println!("  -> 'static' - PARAMS: <Red> <Green> <Blue>");
    println!("  -> 'static_gradient' - PARAMS: <Red1> <Green1> <Blue1> <Red2> <Green2> <Blue2>");
    println!("  -> 'wave_gradient' - PARAMS: <Red1> <Green1> <Blue1> <Red2> <Green2> <Blue2>");
    println!("  -> 'breathing_single' - PARAMS: <Red> <Green> <Blue> <Duration_ms/100>");
    std::process::exit(ret_code);
}

fn main() {
    // Check if socket is OK
    if std::fs::metadata(comms::SOCKET_PATH).is_err() {
        eprintln!("Error. Socket doesn't exit. Is daemon running?");
        std::process::exit(1);
    }
    let mut args : Vec<_> = env::args().collect();
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
            // Special case for setting effect - lots of params
            if args[2].to_ascii_lowercase().as_str() == "effect" {
                args.drain(0..3);
                write_effect(args);
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

fn write_effect(opt: Vec<String>) {
    println!("Write effect: Args: {:?}", opt);
    let name = opt[0].clone();
    let mut params : Vec<u8> = vec![];
    for i in 1..opt.len() {
        if let Ok(x) = opt[i].parse::<u8>() {
            params.push(x)
        } else {
            print_help(format!("Option for effect is not valid (Must be 0-255): `{}`", opt[i]).as_str())
        }
    }
    println!("Params: {:?}", params);
    match name.to_ascii_lowercase().as_str() {
        "static" => {
            if params.len() != 3 { print_help("Static effect requires 3 args") }
            send_effect(name.to_ascii_lowercase(), params)
        }
        "static_gradient" => {
            if params.len() != 6 { print_help("Static gradient requires 6 args") }
            params.push(0); // Until implimented direction
            send_effect(name.to_ascii_lowercase(), params)
        }
        "wave_gradient" => {
            if params.len() != 6 { print_help("Wave gradient requires 6 args") }
            params.push(0); // Until implimented direction
            send_effect(name.to_ascii_lowercase(), params)
        }
        "breathing_single" => {
            if params.len() != 4 { print_help("Breathing single requires 4 args") }
            send_effect(name.to_ascii_lowercase(), params)
        }
        _ => print_help(format!("Unrecognised effect name: `{}`", name).as_str())
    }
}

fn send_effect(name: String, params: Vec<u8>) {
    if let Some(r) = send_data(comms::DaemonCommand::SetEffect { name, params }) {
        if let comms::DaemonResponse::SetEffect { result } = r {
            match result {
                true => println!("Effect set OK!"),
                _ => eprintln!("Effect set FAIL!")
            }
        }
    } else {
        eprintln!("Unknown daemon error!");
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

/*
fn write_colour(r: u8, g: u8, b: u8) {
    if let Some(_) = send_data(comms::DaemonCommand::SetColour { r, g, b }) {
        read_fan_rpm()
    } else {
        eprintln!("Unknown error!");
    }
}
*/