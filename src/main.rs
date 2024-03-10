mod elf;
mod ext;

use crate::elf::Elf;
use clap::Parser;
use indexmap::IndexMap;
use std::{fs::File, path::Path, process::exit};

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

    let path_exe = Path::new(&args.executable);

    // Open the target executable.
    let mut file_exe = match File::open(path_exe) {
        Ok(x) => x,
        Err(e) => {
            println!("Failed to read file \"{}\": {}", &args.executable, e);
            exit(1);
        }
    };
    let mut elf_exe = Elf::read(&mut file_exe).unwrap();

    let mut matched_fns = IndexMap::new();
    // Open and link each library.
    for lib in args.libraries.iter() {
        // Open the library.
        let path_lib = Path::new(&lib);
        let mut file_lib = match File::open(path_lib) {
            Ok(x) => x,
            Err(e) => {
                println!("Failed to read file \"{}\": {}", &lib, e);
                exit(1);
            }
        };
        let elf_lib = Elf::read(&mut file_lib).unwrap();

        // Link the library and print the matched symbols.
        matched_fns.extend(elf_exe.link(&elf_lib));
    }

    // Print which symbols got matched.
    if !args.quiet {
        println!(
            "Linked the following symbols to \"{}\"",
            path_exe.file_name().unwrap().to_string_lossy()
        );
        let import_symbols = elf_exe.get_functions();
        import_symbols.iter().for_each(|(x, _)| {
            println!(
                "[{}]\t{}",
                if matched_fns.contains_key(x) {
                    'X'
                } else {
                    ' '
                },
                x,
            )
        });
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
