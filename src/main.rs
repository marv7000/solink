use elf::Section;

use crate::elf::Elf;

mod elf;
mod ext;

fn main() {
    let mut file1 = std::fs::File::open("/home/marvin/repos/solink/test/build/test_exe").unwrap();
    let mut elf = Elf::read(&mut file1).unwrap();
    elf.add_section(".test_section".to_string(), Section::new(vec![0]))
        .unwrap();
    let mut file2 = std::fs::File::create("a.out").unwrap();
    elf.write(&mut file2).unwrap();
}
