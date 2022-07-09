use std::io;
use std::fs::File;

pub trait Serializable {
    /// Serialize into raw bytes
    fn serialize(&self, f: &mut File) -> io::Result<()>;
}
