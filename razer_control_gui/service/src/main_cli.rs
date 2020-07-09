use std::env;
use std::os::unix::net::UnixStream;
mod core;
use std::io::Write;

fn main() {
    let mut socket : UnixStream = match UnixStream::connect(core::SOCKET_PATH) {
        Ok(sock) => {
            println!("Socket connected!");
            sock
        },
        Err(e) => {
            println!("Couldn't connect to socket: {:?}", e);
            std::process::exit(1);
        }
    };
    socket.write("Test".as_bytes());
}