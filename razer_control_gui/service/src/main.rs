use std::path::Path;
use std::process;
use std::fs;
use std::io::{Error, ErrorKind};
use std::io;
use std::option;

const DRIVER_DIR: &str = "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver";

fn assert_driver_loaded() -> bool {
    return Path::new(DRIVER_DIR).exists();
}


fn get_device_dir() -> Option<String> {
    return match fs::read_dir(DRIVER_DIR).unwrap()
    .find(|x| x.as_ref().unwrap().file_name().to_str().unwrap().starts_with("000"))
    .unwrap() {
        Ok(p) => Some(String::from(p.path().to_str().unwrap())),
        Err(_) => None
    }
}


fn main() {
    if !assert_driver_loaded() {
        println!("Error. Driver is not loaded!");
        process::exit(1);
    }
    let res = get_device_dir();
    if res.is_none() {
        println!("Error. Cannot find device path!");
        process::exit(1);
    }
    let drv_dir = res.unwrap();
    println!("Directory: {}", drv_dir);
    process::exit(0);
}
