mod elf;
mod ext;
mod int;

use crate::elf::Elf;
use clap::Parser;
use indexmap::IndexMap;
use std::{ffi::OsStr, fs::File, path::PathBuf, process::exit};

#[derive(Parser, Debug)]
#[command(version)]
struct Args {
    /// Save the resulting binary at the given location.
    #[arg(short, long)]
    output: Option<PathBuf>,

    /// Only match the given symbol.
    #[arg(short, long)]
    symbols: Vec<String>,

    /// Don't write any messages to the standard output.
    #[arg(short, long)]
    quiet: bool,

    executable: PathBuf,
    libraries: Vec<PathBuf>,
}

fn main() {
    let args = Args::parse();

    // Open the target executable.
    let mut file_exe = match File::open(&args.executable) {
        Ok(x) => x,
        Err(e) => {
            println!(
                "Failed to read file \"{}\": {}",
                args.executable.display(),
                e
            );
            exit(1);
        }
    };
    let mut elf_exe = Elf::read(&mut file_exe).unwrap();

    let mut matched_fns: IndexMap<String, elf::Symbol> = IndexMap::new();
    // Open and link each library.
    for lib in args.libraries.iter() {
        // Open the library.
        let mut file_lib = match File::open(lib) {
            Ok(x) => x,
            Err(e) => {
                println!("Failed to read file \"{}\": {}", lib.display(), e);
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
            args.executable.file_name().unwrap().to_string_lossy()
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
    let output_path = args.output.unwrap_or_else(|| {
        let mut file_stem = args.executable.file_stem().unwrap().to_owned();
        file_stem.push(OsStr::new("_patched"));
        let mut new = args.executable.clone();
        if let Some(ext) = args.executable.extension() {
            new.set_file_name(file_stem);
            new.set_extension(ext);
            new
        } else {
            new.set_file_name(file_stem);
            new
        }
    });

    // Save to output path.
    let mut output = File::create(output_path).unwrap();
    elf_exe.write(&mut output).unwrap();
}
