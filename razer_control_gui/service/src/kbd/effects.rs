use super::*;

///
/// STATIC KEYBOARD EFFECT
/// 1 colour, simple
///

#[derive(Copy, Clone)]
pub struct Static {
    kbd: board::KeyboardData,
    args: [u8; 3],
}

impl Effect for Static {
    fn new(args: Vec<u8>) -> Box<dyn Effect>
    where
        Self: Sized,
    {
        let mut kbd = board::KeyboardData::new();
        kbd.set_kbd_colour(args[0], args[1], args[2]);
        let s = Static { 
            kbd,
            args: [args[0], args[1], args[2]],
        };
        return Box::new(s);
    }

    fn update(&mut self) -> board::KeyboardData {
        return self.kbd;
    }

    fn get_name() -> &'static str
    where
        Self: Sized,
    {
        return "Static";
    }

    fn get_varargs(& mut self) -> &[u8] {
        &self.args
    }

    fn clone_box(&self) -> Box<dyn Effect> {
        return Box::new(self.clone())
    }
}


///
/// STATIC_BLEND KEYBOARD EFFECT
/// 2 colours forming a gradient
///

#[derive(Copy, Clone)]
pub struct StaticGradient {
    kbd: board::KeyboardData,
    args: [u8; 7],
}

impl Effect for StaticGradient {
    fn new(args: Vec<u8>) -> Box<dyn Effect>
    where Self: Sized {
        let mut kbd = board::KeyboardData::new();
        let args : [u8; 7] = [args[0], args[1], args[2], args[3], args[4], args[5], args[6]];
        let mut c1 = board::AnimatorKeyColour { 
            red: args[0] as f32,
            green: args[1] as f32,
            blue: args[2] as f32
        };
        let c2 = board::AnimatorKeyColour { 
            red: args[3] as f32,
            green: args[4] as f32,
            blue: args[5] as f32
        };
        let cDelta = (c2-c1).divide(14.0);
        for i in 0..15 {
            let clamped = c1.get_clamped_colour();
            kbd.set_col_colour(i, clamped.red, clamped.green, clamped.blue);
            c1 += cDelta;
        }

        Box::new(StaticGradient {
            kbd,
            args
        })
    }

    fn update(& mut self) -> board::KeyboardData {
        self.kbd // Nothing to update
    }

    fn get_name() -> &'static str
    where Self: Sized {
        "Static Gradient"
    }

    fn get_varargs(& mut self) -> &[u8] {
        return &self.args;
    }

    fn clone_box(&self) -> Box<dyn Effect> {
        return Box::new(self.clone())
    }
}


///
/// STATIC_BLEND KEYBOARD EFFECT
/// 2 colours forming a gradient, animated across the keyboard
///

#[derive(Copy, Clone)]
pub struct WaveGradient {
    kbd: board::KeyboardData,
    args: [u8; 7],
    colourBand : [board::AnimatorKeyColour; 30],
}

impl Effect for WaveGradient {
    fn new(args: Vec<u8>) -> Box<dyn Effect>
    where Self: Sized {
        let mut kbd = board::KeyboardData::new();
        let args : [u8; 7] = [args[0], args[1], args[2], args[3], args[4], args[5], args[6]];
        let mut wave= WaveGradient {
            kbd,
            args,
            colourBand: [board::AnimatorKeyColour { red: 0.0, green: 0.0, blue: 0.0 }; 30],
        };
        let mut c1 = board::AnimatorKeyColour { 
            red: args[0] as f32,
            green: args[1] as f32,
            blue: args[2] as f32
        };
        let mut c2 = board::AnimatorKeyColour { 
            red: args[3] as f32,
            green: args[4] as f32,
            blue: args[5] as f32
        };
        let cDelta = (c2-c1).divide(15.0);
        for i in 0..15 {
            wave.colourBand[i] = c1;
            c1 += cDelta;
        }
        for i in 0..15 {
            wave.colourBand[i+15] = c2;
            c2 -= cDelta;
        }
        Box::new(wave)
    }

    fn update(& mut self) -> board::KeyboardData {
        for i in 0..15 {
            let c = self.colourBand[i].get_clamped_colour();
            self.kbd.set_col_colour(i, c.red, c.green, c.blue);
        }
        self.colourBand.rotate_right(1);
        self.kbd
    }

    fn get_name() -> &'static str
    where Self: Sized {
        "Wave Gradient"
    }

    fn get_varargs(& mut self) -> &[u8] {
        return &self.args;
    }

    fn clone_box(&self) -> Box<dyn Effect> {
        return Box::new(self.clone())
    }
}

///
/// BREATHING (1 Colour) KEYBOARD EFFECT
/// 1 colour, fading in and out
///
#[derive(Copy, Clone)]
pub struct BreathSingle {
    args: [u8; 4],
    kbd: board::KeyboardData,
    step_duration_ms: u128,
    static_start_ms: u128,
    curr_step: u8, // Step 0 = Off, 1 = increasing, 2 = On, 3 = decreasing
    target_colour: board::AnimatorKeyColour,
    current_colour: board::AnimatorKeyColour,
    animator_step_colour: board::AnimatorKeyColour,
}

impl Effect for BreathSingle {
    fn new(args: Vec<u8>) -> Box<dyn Effect> {
        let mut k = board::KeyboardData::new();
        let cycle_duration_ms = args[3] as f32 * 100.0;
        k.set_kbd_colour(0, 0, 0); // Sets all keyboard lights off initially
        Box::new(BreathSingle {
            args: [args[0], args[1], args[2], args[3]],
            kbd: k,
            step_duration_ms: cycle_duration_ms as u128,
            static_start_ms: get_millis(),
            curr_step: 0,
            target_colour: board::AnimatorKeyColour {
                red: args[0] as f32,
                green: args[1] as f32,
                blue: args[2] as f32,
            },
            current_colour: board::AnimatorKeyColour {
                red: 0.0,
                green: 0.0,
                blue: 0.0,
            },
            animator_step_colour: board::AnimatorKeyColour {
                red: args[0] as f32 / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
                green: args[1] as f32
                    / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
                blue: args[2] as f32 / (cycle_duration_ms as f32 / ANIMATIONS_DELAY_MS as f32) as f32,
            },
        })
    }

    fn update(&mut self) -> board::KeyboardData {
        match self.curr_step {
            0 => {
                self.current_colour = board::AnimatorKeyColour {
                    red: 0.0,
                    green: 0.0,
                    blue: 0.0,
                };
                if get_millis() - self.static_start_ms >= self.step_duration_ms {
                    self.curr_step += 1;
                }
            }
            1 => {
                // Increasing
                self.current_colour += self.animator_step_colour;
                if self.current_colour >= self.target_colour {
                    self.curr_step += 1;
                    self.static_start_ms = get_millis();
                }
            }
            2 => {
                self.current_colour = self.target_colour;
                if get_millis() - self.static_start_ms >= self.step_duration_ms {
                    self.curr_step += 1;
                }
            }
            3 => {
                // Decreasing
                self.current_colour -= self.animator_step_colour;
                let target = board::AnimatorKeyColour {
                    red: 0.0,
                    green: 0.0,
                    blue: 0.0,
                };
                if self.current_colour <= target {
                    self.curr_step = 0;
                    self.static_start_ms = get_millis();
                }
            }
            _ => {} // Unknown state? Ignore
        }
        let col = self.current_colour.get_clamped_colour();
        self.kbd.set_kbd_colour(col.red, col.green, col.blue); // Cast back to u8
        return self.kbd;
    }

    fn get_name() -> &'static str
    where Self: Sized {
        "Breathing Single"
    }

    fn get_varargs(& mut self) -> &[u8] {
        return &self.args;
    }

    fn clone_box(&self) -> Box<dyn Effect> {
        return Box::new(self.clone())
    }
}