# Filed

### A tui file manager with emacs bindings, similar to dired

![image](https://github.com/user-attachments/assets/1a6956ea-3bc9-49f3-9443-6982bc226913)

## Features
- `ls -lah` style output
- navigate filesystem
- minibuffer with subset of emacs bindings

## Usage
- run with `filed <directory>` or `filed` to open in cwd
- `p`          → move up
- `n`          → move down
- `m`          → mark/unmark file
- `d`          → delete marked/selected file(s)
- `r`          → rename selected file
- `C-c`        → exit
- `g`          → refresh
- `o`          → open any directory
- `enter`      → open selected directory
- `+`          → create directory
- `~`          → go to home directory
- `backspace` → go to parent directory
### Modes
- `s`          → soft mode - remove info to prevent wrapping
### Minibuffer
- `C-f`        → forward
- `C-b`        → back
- `C-a`        → begining of line
- `C-e`        → end of line
- `C-d`        → kill next char
- `C-k`        → kill to eol
- `C-g`        → cancel
