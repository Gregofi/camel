pub struct AtomicInt(u32);

impl AtomicInt {
    pub fn new() -> Self {
        AtomicInt(0)
    }

    pub fn get_and_inc(&mut self) -> u32 {
        let x = self.0;
        self.0 += 1;
        x
    }
}
