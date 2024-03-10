use std::io::{Error, Read, Seek, Write};

use crate::elf::Class;

pub trait ReadExt {
    fn read_amount<const BYTES_AMOUNT: usize>(&mut self) -> std::io::Result<[u8; BYTES_AMOUNT]>;
    fn read_class(&mut self, class: &Class) -> std::io::Result<u64>;
    fn read_cstr(&mut self) -> std::io::Result<String>;
}

impl<T: Read> ReadExt for T {
    fn read_amount<const BYTES_AMOUNT: usize>(&mut self) -> std::io::Result<[u8; BYTES_AMOUNT]> {
        let mut x = [0; BYTES_AMOUNT];
        self.read(&mut x)?;
        Ok(x)
    }

    #[inline]
    fn read_class(&mut self, class: &Class) -> std::io::Result<u64> {
        match class {
            Class::Class32 => {
                let mut buf = [0u8; 4];
                self.read(&mut buf)?;
                return Ok(u32::from_ne_bytes(buf) as u64);
            }
            Class::Class64 => {
                let mut buf = [0u8; 8];
                self.read(&mut buf)?;
                return Ok(u64::from_ne_bytes(buf));
            }
        }
    }

    fn read_cstr(&mut self) -> std::io::Result<String> {
        let mut result = String::new();
        let mut c = [0u8];
        loop {
            self.read(&mut c)?;
            if c[0] == 0 {
                break;
            }
            result.push(c[0] as char);
        }
        return Ok(result);
    }
}

pub trait WriteExt {
    fn write_class(&mut self, value: u64, class: &Class) -> std::io::Result<()>;
    fn write_cstr(&mut self, data: &String) -> Result<(), Error>;
}

impl<T: Write> WriteExt for T {
    fn write_cstr(&mut self, data: &String) -> Result<(), Error> {
        self.write(data.as_bytes())?;
        self.write(&[0u8])?;
        Ok(())
    }

    #[inline]
    fn write_class(&mut self, value: u64, class: &Class) -> std::io::Result<()> {
        match class {
            Class::Class32 => {
                self.write(&(value as u32).to_ne_bytes())?;
            }
            Class::Class64 => {
                self.write(&(value as u64).to_ne_bytes())?;
            }
        }
        return Ok(());
    }
}

pub trait SeekExt {
    fn align(&mut self, align: u64) -> std::io::Result<()>;
}

impl<T: Seek> SeekExt for T {
    fn align(&mut self, align: u64) -> std::io::Result<()> {
        let cur = self.stream_position()?;
        if align == 0 {
            return Ok(());
        }

        if cur % align != 0 {
            self.seek(std::io::SeekFrom::Start(cur + (align - cur % align)))?;
        }

        return Ok(());
    }
}

pub fn align(pos: u64, align: u64) -> u64 {
    if align == 0 {
        return pos;
    }
    if pos % align == 0 {
        return pos;
    }
    return pos + (align - pos % align);
}
