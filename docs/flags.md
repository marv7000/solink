# Flags

### `-o <file path>` `--output <file path>`
Save the resulting binary at the given location. 
By default, `solink` wil add a `_patched` suffix to the input executable name.

### `-s <symbol>` `--symbol <symbol>`
Only match the given symbol. 
Multiple `-s` flags can be chained together.

### `-f` `--force`
Forcefully match all external symbols. 
Instead of a warning, the program will exit with a non-zero exit code if one
symbol failed to match.

### `-q` `--quiet`
Don't write any messages to the standard output.

### `-V` `--version`
Write the version to standard output.

### `--help`
Display this message with all flags and their usage. 
If this flag is set, `solink` will not perform any other operation and exit.
