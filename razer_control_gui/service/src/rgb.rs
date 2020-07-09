
// -- RGB Key channel -- 

const KEYS_PER_ROW: usize = 15;
const ROWS: usize = 6;



#[derive(Copy, Clone, Debug)]
pub struct key {
    pub red: u8,
    pub green: u8,
    pub blue: u8
}

#[derive(Copy, Clone, Debug)]
pub struct key_row {
    keys: [key; KEYS_PER_ROW]
}

impl key_row {
    pub fn new() -> key_row {
        return key_row {
            keys: [key{ red: 255, green: 255, blue: 255 }; KEYS_PER_ROW]
        }
    }

    pub fn set_key_color(&mut self, pos: usize, r: u8, g: u8, b: u8) {
        self.keys[pos] = key { red: r, green: g, blue: b }
    }

    pub fn set_row_color(&mut self, r: u8, g: u8, b: u8) {
        (0..KEYS_PER_ROW).for_each(|x| self.set_key_color(x, r, g, b)) // Sets the entire row
    }

    pub fn get_row_data(&mut self) -> Vec<u8> { // *3 as itll be the RGB values
        let mut v: Vec<u8> = vec![];
        self.keys.iter().for_each(|k| {
            v.push(k.red);
            v.push(k.green);
            v.push(k.red);
        });
        return v;
    }
}


#[derive(Copy, Clone, Debug)]
struct keyboard {
    rows: [key_row; ROWS],
    brightness: u8
}

impl keyboard {
    fn get_brightness() {

    }


    pub fn new() -> keyboard {
        return keyboard {
            rows : [key_row::new(); ROWS],
            brightness: 0
        }
    }
}

