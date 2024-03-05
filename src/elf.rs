use byteordered::{ByteOrdered, Endianness};
use std::{
    collections::HashMap,
    error::Error,
    fs::File,
    io::{Read, Seek, SeekFrom},
};

use crate::ext::ReadExt;

#[derive(Debug, Clone)]
pub enum MachineType {
    None = 0,
    // I386 = 3,
    Amd64 = 0x3E,
    // Aarch64 = 183,
    // RiscV = 243
}

#[derive(Debug, Clone)]
pub enum Class {
    Class32 = 1,
    Class64 = 2,
}

#[derive(Debug, Clone)]
pub struct Symbol {
    name: u32,
    info: u8,
    other: u8,
    shndx: u16,
    value: u64,
    size: u64,
}

#[derive(Debug, Clone)]
pub struct SectionHeader {
    sh_name: u32,
    sh_type: u32,
    sh_flags: u64,
    sh_addr: u64,
    sh_offset: u64,
    sh_size: u64,
    sh_link: u32,
    sh_info: u32,
    sh_addralign: u64,
    sh_entsize: u64,
}

#[derive(Debug, Clone)]
pub struct Section {
    header: SectionHeader,
    body: Vec<u8>,
}

impl Section {
    fn new() -> Self {
        Self {
            header: SectionHeader {
                sh_name: 0,
                sh_type: 0,
                sh_flags: 0,
                sh_addr: 0,
                sh_offset: 0,
                sh_size: 0,
                sh_link: 0,
                sh_info: 0,
                sh_addralign: 0,
                sh_entsize: 0,
            },
            body: Vec::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct ProgramHeader {
    p_type: u32,
    p_flags: u32,
    p_offset: u64,
    p_vaddr: u64,
    p_paddr: u64,
    p_filesz: u64,
    p_memsz: u64,
    p_align: u64,
}

#[derive(Debug, Clone)]
pub struct Header {
    ei_magic: [u8; 4],
    ei_class: Class,
    ei_data: Endianness,
    ei_version: u8,
    ei_osabi: u8,
    ei_abiversion: u8,
    ei_pad: [u8; 7],
    e_type: u16,
    e_machine: MachineType,
    e_version: u32,
    e_entry: u64,
    e_phoff: u64,
    e_shoff: u64,
    e_flags: u32,
    e_ehsize: u16,
    e_phentsize: u16,
    e_phnum: u16,
    e_shentsize: u16,
    e_shnum: u16,
    e_shstrndx: u16,
}

#[derive(Debug, Clone)]
pub struct Elf {
    header: Header,
    programs: Vec<ProgramHeader>,
    sections: HashMap<String, Section>,
    symbols: HashMap<String, Symbol>,
    dynamic_symbols: HashMap<String, Symbol>,
}

impl Elf {
    pub fn new() -> Self {
        return Self {
            header: Header {
                ei_magic: [0x7F, 0x45, 0x4C, 0x46],
                ei_class: Class::Class64,
                ei_data: Endianness::Little,
                ei_version: 1,
                ei_osabi: 0,
                ei_abiversion: 0,
                ei_pad: [0; 7],
                e_type: 2,
                e_machine: MachineType::None,
                e_version: 1,
                e_entry: 0,
                e_phoff: 0,
                e_shoff: 0,
                e_flags: 0,
                e_ehsize: 0,
                e_phentsize: 0,
                e_phnum: 0,
                e_shentsize: 0,
                e_shnum: 0,
                e_shstrndx: 0,
            },
            programs: Vec::new(),
            sections: HashMap::new(),
            symbols: HashMap::new(),
            dynamic_symbols: HashMap::new(),
        };
    }

    pub fn read(input: &mut File) -> Result<Self, Box<dyn Error>> {
        let mut result = Elf::new();
        let mut r = ByteOrdered::runtime(input, Endianness::Little);
        let mut old_pos = 0;

        // Read header.
        r.seek(SeekFrom::Start(old_pos))?;
        {
            result.header.ei_magic = r.read_amount::<4>()?;
            result.header.ei_class = match r.read_u8()? {
                1 => Class::Class32,
                2 => Class::Class64,
                _ => todo!(),
            };
            result.header.ei_data = match r.read_u8()? {
                1 => Endianness::Little,
                2 => {
                    r.set_endianness(Endianness::Big);
                    Endianness::Big
                }
                _ => todo!(),
            };
            result.header.ei_version = match r.read_u8()? {
                1 => 1,
                _ => todo!(),
            };
            result.header.ei_osabi = r.read_u8()?;
            result.header.ei_abiversion = r.read_u8()?;
            result.header.ei_pad = r.read_amount::<7>()?;
            result.header.e_type = r.read_u16()?;
            result.header.e_machine = match r.read_u16()? {
                0x3E => MachineType::Amd64,
                _ => MachineType::None,
            };
            result.header.e_version = match r.read_u32()? {
                1 => 1,
                _ => todo!(),
            };
            result.header.e_entry = r.read_class(&result.header.ei_class)?;
            result.header.e_phoff = r.read_class(&result.header.ei_class)?;
            result.header.e_shoff = r.read_class(&result.header.ei_class)?;
            result.header.e_flags = r.read_u32()?;
            result.header.e_ehsize = r.read_u16()?;
            result.header.e_phentsize = r.read_u16()?;
            result.header.e_phnum = r.read_u16()?;
            result.header.e_shentsize = r.read_u16()?;
            result.header.e_shnum = r.read_u16()?;
            result.header.e_shstrndx = r.read_u16()?;
        }

        // Read program headers.
        r.seek(SeekFrom::Start(result.header.e_phoff))?;
        for _ in 0..result.header.e_phnum {
            let prog = match result.header.ei_class {
                Class::Class32 => ProgramHeader {
                    p_type: r.read_u32()?,
                    p_offset: r.read_class(&result.header.ei_class)?,
                    p_vaddr: r.read_class(&result.header.ei_class)?,
                    p_paddr: r.read_class(&result.header.ei_class)?,
                    p_filesz: r.read_class(&result.header.ei_class)?,
                    p_memsz: r.read_class(&result.header.ei_class)?,
                    p_flags: r.read_u32()?,
                    p_align: r.read_class(&result.header.ei_class)?,
                },
                Class::Class64 => ProgramHeader {
                    p_type: r.read_u32()?,
                    p_flags: r.read_u32()?,
                    p_offset: r.read_class(&result.header.ei_class)?,
                    p_vaddr: r.read_class(&result.header.ei_class)?,
                    p_paddr: r.read_class(&result.header.ei_class)?,
                    p_filesz: r.read_class(&result.header.ei_class)?,
                    p_memsz: r.read_class(&result.header.ei_class)?,
                    p_align: r.read_class(&result.header.ei_class)?,
                },
            };
            result.programs.push(prog);
        }

        // Read sections.
        let mut sections = Vec::new();
        r.seek(SeekFrom::Start(result.header.e_shoff))?;
        for _ in 0..result.header.e_shnum {
            // Read section header.
            let section_header = SectionHeader {
                sh_name: r.read_u32()?,
                sh_type: r.read_u32()?,
                sh_flags: r.read_class(&result.header.ei_class)?,
                sh_addr: r.read_class(&result.header.ei_class)?,
                sh_offset: r.read_class(&result.header.ei_class)?,
                sh_size: r.read_class(&result.header.ei_class)?,
                sh_link: r.read_u32()?,
                sh_info: r.read_u32()?,
                sh_addralign: r.read_class(&result.header.ei_class)?,
                sh_entsize: r.read_class(&result.header.ei_class)?,
            };

            // Read section body.
            old_pos = r.stream_position()?;
            let mut section_body = vec![0u8; section_header.sh_size as usize];
            r.seek(SeekFrom::Start(section_header.sh_offset))?;
            r.read_exact(&mut section_body)?;

            let sect = Section {
                header: section_header,
                body: section_body,
            };
            sections.push(sect);
            r.seek(SeekFrom::Start(old_pos))?;
        }
        let string_table = &sections[result.header.e_shstrndx as usize];

        // Resolve section names.
        for sect in &sections {
            r.seek(SeekFrom::Start(
                string_table.header.sh_offset + sect.header.sh_name as u64,
            ))?;
            let name = r.read_cstr()?;
            if !name.is_empty() {
                result.sections.insert(name, sect.clone());
            }
        }

        // Read symbol table.
        let mut symbols = Vec::new();
        let symtab = match result.sections.get(".symtab") {
            Some(x) => x,
            None => panic!("Section \".symtab\" not found!"),
        };
        r.seek(SeekFrom::Start(symtab.header.sh_offset))?;

        let symbol_size = symtab.header.sh_size / symtab.header.sh_entsize;
        for _ in 0..symbol_size {
            let symbol = Symbol {
                name: r.read_u32()?,
                info: r.read_u8()?,
                other: r.read_u8()?,
                shndx: r.read_u16()?,
                value: r.read_class(&result.header.ei_class)?,
                size: r.read_class(&result.header.ei_class)?,
            };
            symbols.push(symbol);
        }

        // Resolve symbol names.
        let strtab = match result.sections.get(".strtab") {
            Some(x) => x,
            None => panic!("Section \".strtab\" not found!"),
        };
        for symbol in &symbols {
            r.seek(SeekFrom::Start(
                strtab.header.sh_offset + symbol.name as u64,
            ))?;
            let name = r.read_cstr()?;
            if !name.is_empty() {
                result.symbols.insert(name, symbol.clone());
            }
        }
        r.seek(SeekFrom::Start(0))?;

        // Read dynamic symbol table.
        let mut dynamic_symbols = Vec::new();
        let dynsym = match result.sections.get(".dynsym") {
            Some(x) => x,
            None => panic!("Section \".dynsym\" not found!"),
        };
        r.seek(SeekFrom::Start(dynsym.header.sh_offset))?;

        let dynamic_symbol_size = dynsym.header.sh_size / dynsym.header.sh_entsize;
        for _ in 0..dynamic_symbol_size {
            let symbol = Symbol {
                name: r.read_u32()?,
                info: r.read_u8()?,
                other: r.read_u8()?,
                shndx: r.read_u16()?,
                value: r.read_class(&result.header.ei_class)?,
                size: r.read_class(&result.header.ei_class)?,
            };
            dynamic_symbols.push(symbol);
        }

        // Resolve dynamic symbol names.
        let dynstr = match result.sections.get(".dynstr") {
            Some(x) => x,
            None => panic!("Section \".dynstr\" not found!"),
        };
        for symbol in &dynamic_symbols {
            r.seek(SeekFrom::Start(
                dynstr.header.sh_offset + symbol.name as u64,
            ))?;
            let name = r.read_cstr()?;
            if !name.is_empty() {
                result.dynamic_symbols.insert(name, symbol.clone());
            }
        }
        r.seek(SeekFrom::Start(0))?;

        return Ok(result);
    }
}
