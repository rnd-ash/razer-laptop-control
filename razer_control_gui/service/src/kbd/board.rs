use crate::driver_sysfs;
use std::cmp::Ordering;
use std::ops;


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
    pub blue: u8,
}

/// Same as `KeyColour`, but uses f32 values, for more accurate frame by frame
/// colour blending in animations
#[derive(Copy, Clone, Debug)]
pub struct AnimatorKeyColour {
    pub red: f32,
    pub green: f32,
    pub blue: f32,
}

impl AnimatorKeyColour {
    fn new(red: f32, green: f32, blue: f32) -> AnimatorKeyColour {
        AnimatorKeyColour { red, green, blue }
    }

    /// Clamps a f32 between 0 and 255, returns a `u8`
    fn clamp_colour(inp: f32) -> u8 {
        let mut input = inp;
        if input > 255.0 {
            input = 255.0
        };
        if input < 0.0 {
            input = 0.0
        };
        return input as u8;
    }

    pub fn divide(&mut self, divisor: f32) -> AnimatorKeyColour {
        AnimatorKeyColour {
            red: self.red / divisor,
            green: self.green / divisor,
            blue: self.blue / divisor
        }
    }

    pub fn get_clamped_colour(&self) -> KeyColour {
        KeyColour {
            red: AnimatorKeyColour::clamp_colour(self.red),
            green: AnimatorKeyColour::clamp_colour(self.green),
            blue: AnimatorKeyColour::clamp_colour(self.blue),
        }
    }
}

impl ops::Add for AnimatorKeyColour {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self {
            red: self.red + rhs.red,
            green: self.green + rhs.green,
            blue: self.blue + rhs.blue,
        }
    }
}

impl ops::Sub for AnimatorKeyColour {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self {
            red: self.red - rhs.red,
            green: self.green - rhs.green,
            blue: self.blue - rhs.blue,
        }
    }
}

impl ops::AddAssign for AnimatorKeyColour {
    fn add_assign(&mut self, rhs: AnimatorKeyColour) {
        self.red += rhs.red;
        self.green += rhs.green;
        self.blue += rhs.blue;
    }
}

impl ops::SubAssign for AnimatorKeyColour {
    fn sub_assign(&mut self, rhs: AnimatorKeyColour) {
        self.red -= rhs.red;
        self.green -= rhs.green;
        self.blue -= rhs.blue;
    }
}

impl PartialEq for AnimatorKeyColour {
    fn eq(&self, other: &AnimatorKeyColour) -> bool {
        self.red == other.red && self.blue == other.blue && self.green == other.green
    }
}

impl PartialOrd for AnimatorKeyColour {
    fn partial_cmp(&self, other: &AnimatorKeyColour) -> Option<Ordering> {
        if self.red == other.red && self.blue == other.blue && self.green == other.green {
            return Some(Ordering::Equal);
        } else if self.red >= other.red && self.blue >= other.blue && self.green >= other.green {
            return Some(Ordering::Greater);
        } else if self.red <= other.red && self.blue <= other.blue && self.green <= other.green {
            return Some(Ordering::Less);
        }
        return None;
    }
}

#[derive(Copy, Clone, Debug)]
/// Represents a horizontal row of 15 keys on the keyboard
pub struct RowData {
    keys: [KeyColour; KEYS_PER_ROW],
}

impl RowData {
    /// Generates an empty keyboard row, with each key being white (FF,FF,FF)
    pub fn new() -> RowData {
        return RowData {
            keys: [KeyColour {
                red: 255,
                green: 255,
                blue: 255,
            }; KEYS_PER_ROW],
        };
    }

    /// Sets key colour within the row
    ///
    /// # Parameters
    /// * pos - Key number within the matrix, starting from left side of the keyboard
    /// * r - Red channel value
    /// * g - Green channel value
    /// * b - Blue channel value
    pub fn set_key_color(&mut self, pos: usize, r: u8, g: u8, b: u8) {
        self.keys[pos] = KeyColour {
            red: r,
            green: g,
            blue: b,
        }
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

    pub fn get_row_data(&mut self) -> Vec<u8> {
        // *3 as itll be the RGB values
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
    brightness: u8,
}

impl KeyboardData {
    pub fn new() -> KeyboardData {
        return KeyboardData {
            rows: [RowData::new(); ROWS],
            brightness: 0,
        };
    }

    pub fn set_brightness(&mut self, val: u8) -> bool {
        driver_sysfs::write_brightness(val)
    }

    pub fn get_brightness(&mut self) -> u8 {
        self.brightness = driver_sysfs::read_brightness();
        self.brightness
    }

    pub fn update_kbd(&mut self) -> bool {
        let mut all_vals = Vec::<u8>::with_capacity(3 * KEYS_PER_ROW * ROWS);
        for row in self.rows.iter_mut() {
            all_vals.extend(&row.get_row_data());
        }
        driver_sysfs::write_rgb_map(all_vals)
    }

    /// Sets a specific key in the keyboard matrix to a colour
    pub fn set_key_colour(&mut self, row: usize, col: usize, r: u8, g: u8, b: u8) {
        if row >= ROWS {
            return;
        }
        if col >= KEYS_PER_ROW {
            return;
        }
        self.rows[row].set_key_color(col, r, g, b)
    }

    /// Sets a horizontal row on the keyboard to a colour
    pub fn set_row_colour(&mut self, row: usize, r: u8, g: u8, b: u8) {
        if row >= ROWS {
            return;
        }
        self.rows[row].set_row_color(r, g, b)
    }

    /// Sets a vertical column on the keyboard to a colour
    pub fn set_col_colour(&mut self, col: usize, r: u8, g: u8, b: u8) {
        if col >= KEYS_PER_ROW {
            return;
        }
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

    /// Returns a specific key
    pub fn get_key_at(self, index: usize) -> KeyColour {
        self.rows[index / KEYS_PER_ROW].keys[index % KEYS_PER_ROW]
    }

    /// Internal function used only for the combining of effect layers
    pub fn set_key_at(&mut self, index: usize, col: KeyColour) {
        self.rows[index / KEYS_PER_ROW].keys[index % KEYS_PER_ROW] = col
    }
}
