# TBA - Control application for RGB / power mode

This is a Rust based project to create a daemon, cli, and GUI application to control razer keyboards using my driver

## Daemon
This is a daemon that sits between any programs wanting to interface with the hardware and the kernel module. This daemon
is responsible for saving settings (Power modes, RGB effects), and listen for socket events (at /tmp/razersocket) from
any userspace applications, and process the request

### Daemon - What works
* Setup of socket
* Receiving commands
* Configuration saving and loading (See /usr/share/razercontrol/ for json saves)

### Daemon - What is to do
* Actual API (mod) to send commands to daemon (For custom apps)
* Background keyboard animations - Rust won't build due to borrow issues right now


## CLI App
TBA - Incomplete

## GUI
TBA - Not started
