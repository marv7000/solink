use byteordered::{ByteOrdered, Endianness};
use indexmap::IndexMap;
use num_enum::FromPrimitive;
use std::{
    error::Error,
    fs::File,
    io::{Read, Seek, SeekFrom, Write},
};

use crate::ext::{ReadExt, SeekExt, WriteExt};

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

#[derive(FromPrimitive, Debug, Clone, Default)]
#[repr(u32)]
pub enum PhType {
    Load = 0x1,
    Dynamic = 0x2,
    Inerp = 0x3,
    Note = 0x4,
    Shlib = 0x5,
    Phdr = 0x6,
    Tls = 0x7,
    Num = 0x8,
    #[default]
    Unknown = 0x0,
}

#[derive(Debug, Clone)]
pub struct Symbol {
    sym_name: u32,
    sym_info: u8,
    sym_other: u8,
    sym_shndx: u16,
    sym_value: u64,
    sym_size: u64,
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
    pub fn new(data: Vec<u8>) -> Self {
        Self {
            header: SectionHeader {
                sh_name: 0,
                sh_type: 0,
                sh_flags: 0,
                sh_addr: 0,
                sh_offset: 0,
                sh_size: data.len() as u64,
                sh_link: 0,
                sh_info: 0,
                sh_addralign: 0,
                sh_entsize: 1,
            },
            body: data,
        }
    }
}

#[derive(Debug, Clone)]
pub struct ProgramHeader {
    p_type: PhType,
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
    pub ei_magic: [u8; 4],
    pub ei_class: Class,
    pub ei_data: Endianness,
    pub ei_version: u8,
    pub ei_osabi: u8,
    pub ei_abiversion: u8,
    pub ei_pad: [u8; 7],
    pub e_type: u16,
    pub e_machine: MachineType,
    pub e_version: u32,
    pub e_entry: u64,
    pub e_phoff: u64,
    pub e_shoff: u64,
    pub e_flags: u32,
    pub e_ehsize: u16,
    pub e_phentsize: u16,
    pub e_phnum: u16,
    pub e_shentsize: u16,
    pub e_shnum: u16,
    pub e_shstrndx: u16,
}

#[derive(Debug, Clone)]
pub struct Elf {
    pub header: Header,
    pub programs: Vec<ProgramHeader>,
    pub sections: IndexMap<String, Section>,
    pub symbols: IndexMap<String, Symbol>,
    pub dynamic_symbols: IndexMap<String, Symbol>,
    pub string_table: Section,
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
                e_ehsize: 0x40,
                e_phentsize: 0x38,
                e_phnum: 1,
                e_shentsize: 0x40,
                e_shnum: 1,
                e_shstrndx: 0,
            },
            programs: vec![],
            sections: IndexMap::new(),
            symbols: IndexMap::new(),
            dynamic_symbols: IndexMap::new(),
            string_table: Section::new(vec![]),
        };
    }

    /// Returns an IndexMap with names and symbol references.
    pub fn get_functions(&self) -> IndexMap<String, Symbol> {
        self.dynamic_symbols
            .clone()
            .into_iter()
            .filter(|(x, y)| self.dynamic_symbols.contains_key(x.as_str()) && y.sym_info == 0x12)
            .collect::<IndexMap<String, Symbol>>()
    }

    /// Links an ELF to self.
    pub fn link(&mut self, other: &Elf) {
        // Get all exported functions from the other ELF.
        let matched_fns = other.get_functions();

        // Get all imported functions from
        dbg!(&matched_fns);
        // TODO
    }

    pub fn update(&mut self) -> Result<(), Box<dyn Error>> {
        // Get base offset (ELF header + program headers).
        let base = self.header.e_ehsize as u64
            + (self.header.e_phentsize as u64 * self.header.e_phnum as u64);

        // Update program header sizes + offsets.
        // TODO

        // Update section sizes + offsets.
        let mut section_pos = base;
        for (_, section) in &mut self.sections {
            section.header.sh_offset = section_pos;
            // Add the size of this section to the current cursor.
            section_pos += section.body.len() as u64;
            // Align.
            if section.header.sh_addralign != 0 && section_pos % section.header.sh_addralign != 0 {
                section_pos +=
                    section.header.sh_addralign - section_pos % section.header.sh_addralign;
            }
        }

        // Start of the symbol table is after all sections.
        self.header.e_shoff = section_pos;

        // Update symbol table.
        {
            let mut symtab_data = Vec::new();
            let mut w = ByteOrdered::runtime(&mut symtab_data, self.header.ei_data);
            for (_, symbol) in &mut self.symbols {
                match &self.header.ei_class {
                    Class::Class32 => {
                        w.write_u32(symbol.sym_name)?;
                        w.write_class(symbol.sym_value, &self.header.ei_class)?;
                        w.write_class(symbol.sym_size, &self.header.ei_class)?;
                        w.write_u8(symbol.sym_info)?;
                        w.write_u8(symbol.sym_other)?;
                        w.write_u16(symbol.sym_shndx)?;
                    }
                    Class::Class64 => {
                        w.write_u32(symbol.sym_name)?;
                        w.write_u8(symbol.sym_info)?;
                        w.write_u8(symbol.sym_other)?;
                        w.write_u16(symbol.sym_shndx)?;
                        w.write_class(symbol.sym_value, &self.header.ei_class)?;
                        w.write_class(symbol.sym_size, &self.header.ei_class)?;
                    }
                };
            }
            self.string_table.header.sh_size = symtab_data.len() as u64;
            self.string_table.body = symtab_data;
        }

        // Update symbol string table.
        {
            let mut strtab_data = vec![0u8];
            let mut strtab_pos = 1; // Leave room for null strings.
            for (name, symbol) in &mut self.symbols {
                strtab_data.write_cstr(name)?;
                symbol.sym_name = strtab_pos as u32;
                strtab_pos += name.len() + 1;
            }
            self.string_table.body = strtab_data;
            self.string_table.header.sh_size = strtab_pos as u64;
        }

        // Update section string table.
        {
            let mut shstr_data = vec![0u8];
            let mut shstr_pos = 1; // Leave room for null strings.
            for (name, section) in &mut self.sections {
                shstr_data.write_cstr(name)?;
                section.header.sh_name = shstr_pos as u32;
                shstr_pos += name.len() + 1;
            }
            self.header.e_shstrndx = self.sections.get_index_of(".shstrtab").unwrap() as u16;
            let shstrtab = &mut self.sections[".shstrtab"];
            shstrtab.body = shstr_data;
            shstrtab.header.sh_size = shstr_pos as u64;
        }

        Ok(())
    }

    pub fn read(input: &mut File) -> Result<Self, Box<dyn Error>> {
        let mut result = Elf::new();
        let mut r = ByteOrdered::runtime(input, Endianness::Little);
        let mut old_pos = 0;

        // Read header.
        {
            r.seek(SeekFrom::Start(old_pos))?;
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
                    p_type: PhType::from(r.read_u32()?),
                    p_offset: r.read_class(&result.header.ei_class)?,
                    p_vaddr: r.read_class(&result.header.ei_class)?,
                    p_paddr: r.read_class(&result.header.ei_class)?,
                    p_filesz: r.read_class(&result.header.ei_class)?,
                    p_memsz: r.read_class(&result.header.ei_class)?,
                    p_flags: r.read_u32()?,
                    p_align: r.read_class(&result.header.ei_class)?,
                },
                Class::Class64 => ProgramHeader {
                    p_type: PhType::from(r.read_u32()?),
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
            match name.as_str() {
                ".strtab" => result.string_table = sect.clone(),
                "" => (), // Don't insert empty keys.
                _ => _ = result.sections.insert(name, sect.clone()),
            };
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
            let symbol = match &result.header.ei_class {
                Class::Class64 => Symbol {
                    sym_name: r.read_u32()?,
                    sym_value: r.read_class(&result.header.ei_class)?,
                    sym_size: r.read_class(&result.header.ei_class)?,
                    sym_info: r.read_u8()?,
                    sym_other: r.read_u8()?,
                    sym_shndx: r.read_u16()?,
                },
                Class::Class32 => Symbol {
                    sym_name: r.read_u32()?,
                    sym_info: r.read_u8()?,
                    sym_other: r.read_u8()?,
                    sym_shndx: r.read_u16()?,
                    sym_value: r.read_class(&result.header.ei_class)?,
                    sym_size: r.read_class(&result.header.ei_class)?,
                },
            };
            symbols.push(symbol);
        }

        // Resolve symbol names.
        for symbol in &symbols {
            r.seek(SeekFrom::Start(
                result.string_table.header.sh_offset + symbol.sym_name as u64,
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
                sym_name: r.read_u32()?,
                sym_info: r.read_u8()?,
                sym_other: r.read_u8()?,
                sym_shndx: r.read_u16()?,
                sym_value: r.read_class(&result.header.ei_class)?,
                sym_size: r.read_class(&result.header.ei_class)?,
            };
            dynamic_symbols.push(symbol);
        }

        // Resolve dynamic symbol names.
        let dynstr = match result.sections.get(".dynstr") {
            Some(x) => x,
            None => panic!("Section \".dynstr\" not found!"),
        };
        let dynstr_off = dynstr.header.sh_offset;
        for symbol in &dynamic_symbols {
            r.seek(SeekFrom::Start(dynstr_off + symbol.sym_name as u64))?;
            let name = r.read_cstr()?;
            if !name.is_empty() {
                result.dynamic_symbols.insert(name, symbol.clone());
            }
        }
        r.seek(SeekFrom::Start(0))?;

        // Finalize.
        result.update()?;
        return Ok(result);
    }

    pub fn write(&mut self, output: &mut File) -> Result<(), Box<dyn Error>> {
        let mut w = ByteOrdered::runtime(output, Endianness::Little);
        self.update()?;

        // Write header.
        w.write_all(&self.header.ei_magic)?;
        w.write_u8(self.header.ei_class.clone() as u8)?;
        w.write_u8(self.header.ei_data as u8 + 1)?; // 1 is Little, 2 is Big.
        w.write_u8(self.header.ei_version)?;
        w.write_u8(self.header.ei_osabi)?;
        w.write_u8(self.header.ei_abiversion)?;
        w.write_all(&self.header.ei_pad)?;
        w.set_endianness(self.header.ei_data);
        w.write_u16(self.header.e_type)?;
        w.write_u16(self.header.e_machine.clone() as u16)?;
        w.write_u32(self.header.e_version)?;
        w.write_class(self.header.e_entry, &self.header.ei_class)?;
        w.write_class(self.header.e_phoff, &self.header.ei_class)?;
        w.write_class(self.header.e_shoff, &self.header.ei_class)?;
        w.write_u32(self.header.e_flags)?;
        w.write_u16(self.header.e_ehsize)?;
        w.write_u16(self.header.e_phentsize)?;
        w.write_u16(self.programs.len() as u16)?;
        w.write_u16(self.header.e_shentsize)?;
        w.write_u16(self.sections.len() as u16)?;
        w.write_u16(self.header.e_shstrndx)?;

        // Write program headers.
        w.seek(SeekFrom::Start(self.header.e_phoff))?;
        for program in &self.programs {
            w.write_u32(program.p_type.clone() as u32)?;
            match &self.header.ei_class {
                Class::Class64 => w.write_u32(program.p_flags)?,
                Class::Class32 => (),
            };
            w.write_class(program.p_offset, &self.header.ei_class)?;
            w.write_class(program.p_vaddr, &self.header.ei_class)?;
            w.write_class(program.p_paddr, &self.header.ei_class)?;
            w.write_class(program.p_filesz, &self.header.ei_class)?;
            w.write_class(program.p_memsz, &self.header.ei_class)?;
            match &self.header.ei_class {
                Class::Class64 => (),
                Class::Class32 => w.write_u32(program.p_flags)?,
            };
            w.write_class(program.p_align, &self.header.ei_class)?;
        }

        // Write section bodies.
        for (_, section) in &self.sections {
            w.align(section.header.sh_addralign)?;
            w.write(&section.body)?;
        }

        // Write section headers.
        w.seek(SeekFrom::Start(self.header.e_shoff))?;
        for (_, section) in &self.sections {
            w.write_u32(section.header.sh_name)?;
            w.write_u32(section.header.sh_type)?;
            w.write_class(section.header.sh_flags, &self.header.ei_class)?;
            w.write_class(section.header.sh_addr, &self.header.ei_class)?;
            w.write_class(section.header.sh_offset, &self.header.ei_class)?;
            w.write_class(section.header.sh_size, &self.header.ei_class)?;
            w.write_u32(section.header.sh_link)?;
            w.write_u32(section.header.sh_info)?;
            w.write_class(section.header.sh_addralign, &self.header.ei_class)?;
            w.write_class(section.header.sh_entsize, &self.header.ei_class)?;
        }

        return Ok(());
    }
}
