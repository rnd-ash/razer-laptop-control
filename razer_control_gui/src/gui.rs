#![cfg_attr(not(feature = "gtk_3_10"), allow(unused_variables, unused_mut))]
extern crate gdk;
extern crate gio;
extern crate glib;
extern crate gtk;

use gio::prelude::*;
use glib::clone;
use gtk::prelude::*;
use gtk::{
    AboutDialog, AppChooserDialog, ApplicationWindow, Builder, Button, Dialog, Entry,
    FileChooserAction, FileChooserDialog, FontChooserDialog, RecentChooserDialog, ResponseType,
    Scale, SpinButton, Spinner, Switch, Window,
};
use std::env::args;

fn build_ui(application: &gtk::Application) {
    let glade_src = include_str!("ui/test.glade");
    let builder = Builder::new_from_string(glade_src);
    let window: ApplicationWindow = builder.get_object("settings_panel").expect("Couldn't get window");
    window.set_application(Some(application));
    window.show_all();
}


fn main() {
    let application = gtk::Application::new(Some("com.rndash.razercontrol"), Default::default())
        .expect("Failed to init GUI");
    application.connect_activate(|app| {
        build_ui(app);
    });
    application.run(&args().collect::<Vec<_>>());
}
