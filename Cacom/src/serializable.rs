use std::io;
use std::io::prelude::*;
use std::fs::File;

pub trait Serializable {
    /// Serialize into raw bytes
    fn serialize(&self, f: &mut File) -> io::Result<()>;
}
