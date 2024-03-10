mod elf;
mod ext;

use crate::elf::Elf;
use clap::Parser;
use std::fs::File;

#[derive(Parser, Debug)]
#[command(version, about, long_about)]
struct Args {
    #[arg(short, long, help = "Save the resulting binary at the given location.")]
    output: Option<String>,

    #[arg(short, long, help = "Only match the given symbol.")]
    symbols: Vec<String>,

    #[arg(
        short,
        long,
        default_value = "false",
        help = "Don't write any messages to the standard output."
    )]
    quiet: bool,

    executable: String,
    libraries: Vec<String>,
}

fn main() {
    let args = Args::parse();

    // Open the target executable.
    let mut file_exe = File::open(&args.executable).unwrap();
    let mut elf_exe = Elf::read(&mut file_exe).unwrap();

    // Open and link each library.
    for lib in args.libraries {
        let mut file_lib = File::open(lib).unwrap();
        let elf_lib = Elf::read(&mut file_lib).unwrap();

        elf_exe.link(&elf_lib);
    }

    // Get output name or create one.
    let output_path = match args.output {
        Some(x) => x,
        None => args.executable.clone() + "_patched",
    };

    // Save to output path.
    let mut output = File::create(output_path).unwrap();
    elf_exe.write(&mut output).unwrap();
}
