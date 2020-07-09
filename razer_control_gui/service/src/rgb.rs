use crate::core;

// -- RGB Key channel -- 

const KEYS_PER_ROW: usize = 15;
const ROWS: usize = 6;



#[derive(Copy, Clone, Debug)]
/// Represents the colour channels for a key
pub struct KeyColour {
    /// Red channel
    pub red: u8,
    /// Green channel
    pub green: u8,
    /// Blue channel
    pub blue: u8
}

#[derive(Copy, Clone, Debug)]
/// Represents a horizontal row of 15 keys on the keyboard
pub struct RowData {
    keys: [KeyColour; KEYS_PER_ROW]
}

impl RowData {
    /// Generates an empty keyboard row, with each key being white (FF,FF,FF)
    pub fn new() -> RowData {
        return RowData {
            keys: [KeyColour{ red: 255, green: 255, blue: 255 }; KEYS_PER_ROW]
        }
    }

    /// Sets key colour within the row
    ///
    /// # Parameters
    /// * pos - Key number within the matrix, starting from left side of the keyboard
    /// * r - Red channel value
    /// * g - Green channel value
    /// * b - Blue channel value
    pub fn set_key_color(&mut self, pos: usize, r: u8, g: u8, b: u8) {
        self.keys[pos] = KeyColour { red: r, green: g, blue: b }
    }

    /// Sets the entire key row to a colour
    ///
    /// # Parameters
    /// * r - Red channel value
    /// * g - Green channel value
    /// * b - Blue channel value
    pub fn set_row_color(&mut self, r: u8, g: u8, b: u8) {
        (0..KEYS_PER_ROW).for_each(|x| self.set_key_color(x, r, g, b)) // Sets the entire row
    }

    pub fn get_row_data(&mut self) -> Vec<u8> { // *3 as itll be the RGB values
        let mut v = Vec::<u8>::with_capacity(3 * KEYS_PER_ROW);
        self.keys.iter().for_each(|k| {
            v.push(k.red);
            v.push(k.green);
            v.push(k.blue);
        });
        return v;
    }
}


#[derive(Copy, Clone, Debug)]
pub struct KeyboardData {
    rows: [RowData; ROWS],
    brightness: u8
}

impl KeyboardData {

    pub fn new() -> KeyboardData {
        return KeyboardData {
            rows : [RowData::new(); ROWS],
            brightness: 0
        }
    }

    pub fn set_brightness(&mut self, val: u8, handler: &mut core::DriverHandler) -> bool {
        handler.write_brightness(val)
    }

    pub fn get_brightness(&mut self, handler: &mut core::DriverHandler) -> u8 {
        self.brightness = handler.read_brightness();
        self.brightness
    }

    pub fn update_kbd(&mut self, handler: &mut core::DriverHandler) -> bool {
        let mut all_vals = Vec::<u8>::with_capacity(3 * KEYS_PER_ROW * ROWS);
        for row in self.rows.iter_mut() {
            all_vals.extend(&row.get_row_data());
        }
        handler.write_rgb_map(all_vals)
    }

    /// Sets a specific key in the keyboard matrix to a colour
    pub fn set_key_colour(&mut self, row: usize, col: usize, r: u8, g: u8, b: u8) {
        self.rows[row].set_key_color(col, r, g, b)
    }

    /// Sets a horizontal row on the keyboard to a colour
    pub fn set_row_colour(&mut self, row: usize, r: u8, g: u8, b: u8) {
        self.rows[row].set_row_color(r, g, b)
    }

    /// Sets a vertical column on the keyboard to a colour
    pub fn set_col_colour(&mut self, col: usize, r: u8, g: u8, b: u8) {
        for row_id in 0..ROWS {
            self.rows[row_id].set_key_color(col, r, g, b)
        }
    }

    /// Sets the entire keyboard to a colour
    pub fn set_kbd_colour(&mut self, r: u8, g: u8, b: u8) {
        for row_id in 0..ROWS {
            self.rows[row_id].set_row_color(r, g, b)
        }
    }

}

