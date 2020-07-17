# Experimental code in here!

## Current features
* Full background daemon - Auto load state on machine startup based on last configuration
* CLI application for adjusting basic settings

## Installing
1. Ensure you have at **least** version 1.2.0 of the kernel module installed
2. Install cargo or rustc
3. run install.sh as a normal user
4. Enjoy!

## Usage of CLI Application
```
razer-cli <action> <attribute> <args> 
```

### action
* read - Read an attribute (get its current state) - No additional args are supplied
* write - Write an attribute, and save it to configuration - See below for argument counts

### attribute
* fan - Fan RPM. ARG: 0 = Auto, anything else is interpreted as a litteral RPM
* power - Power mode. ARG: 0 = Balanced, 1 = Gaming, 2 = Creator
* colour - Keyboard colour. ARGS: R G B channels, each channel is set from 0 to 255
