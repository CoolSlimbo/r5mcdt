# R5Reloaded Mod Creator and Deconstructor Tool

## Purpose
This is a different to use mods for R5Reloaded Mod Manager.

## To Use
### Mod Creation
To create a mod, select 1.
You will then be prompted to fill in the following details (`*` is required):
- Display Name*`
- Internal Name`*`
- Description
- SDK Version`*`
- Version
- Author
- The folder to use`*` - Use `./` if you'd like to convert the current directory into a mod(will ignore executable), or specify a folder relative to the executable to make into a mod.
- File name of mod - Leave blank 
> You'll be prompted to continue, type `y` to build the mod, or `n` to return to the main screen.

### Mod Deconstruction
To deconstruct an existing mod, select 2.
You will be asked to specify the mod file to use. This is relative to the executable.
You will then be asked if you'd only like the manifest.txt - this includes the names, description, etc. Typing `n` or nothing will generate the files.
After it's generated, the directory will be pritned to the console.

### Mod Editing
> *This is not fully implimented as of yet*

## Building
The only extra resource required to build is `Crypto++`. The source for it isn't located with this project and must be self built and placed in `libs\`
