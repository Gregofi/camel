use std::fs::File;
use std::io;

pub trait Serializable {
    /// Serialize into raw bytes
    fn serialize(&self, f: &mut File) -> io::Result<()>;
}
