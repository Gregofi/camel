use core::fmt;
use std::{fmt::Display, fs, io::Write};

use lazy_static::lazy_static;
use regex::Regex;

use crate::serializable::Serializable;

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

/// Generates unique labels
pub struct LabelGenerator {
    counter: AtomicInt,
}

impl LabelGenerator {
    pub fn new() -> Self {
        LabelGenerator {
            counter: AtomicInt::new(),
        }
    }

    /// Creates unique label with given prefix, the prefix can't
    /// contain underscore followed by number at the end.
    pub fn get_label(&mut self, str: &'static str) -> String {
        lazy_static! {
            static ref RE: Regex = Regex::new(r".*(_\d+)$").unwrap();
        }
        if RE.is_match(str) {
            panic!("string label cannot contain underscore");
        }
        format!("{}_{}", str, self.counter.get_and_inc())
    }
}

/// Beginning and end (in byte offset) of the token.
#[derive(Debug, Clone, Eq, PartialEq, Copy)]
pub struct Location(pub usize, pub usize);

impl Location {
    fn new(begin: usize, end: usize) -> Self {
        Location(begin, end)
    }
}

impl Serializable for Location {
    fn serialize(&self, f: &mut fs::File) -> std::io::Result<()> {
        f.write_all(&self.0.to_le_bytes())?;
        f.write_all(&self.1.to_le_bytes())
    }
}

impl Display for Location {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", self.0, self.1)
    }
}
