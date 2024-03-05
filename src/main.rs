use crate::elf::Elf;

mod elf;
mod ext;

fn main() {
    let mut file = std::fs::File::open("/home/marvin/repos/solink/test/build/test_exe").unwrap();
    let elf = Elf::read(&mut file).unwrap();
    dbg!(&elf);
}
