# Test program to control Razer blades

**You must have the driver installed to use this!**

## Building
```
cmake .
make
```

## Usage
###To control fan RPM
Auto RPM 
```
sudo ./rgb_test --fan auto 
```

Manual RPM
```
sudo ./rgb_test --fan 4000 
```
### Power target
Balanced power
```
sudo ./rgb_test --power balanced
```
Gaming mode
```
sudo ./rgb_test --power gaming
```
Creator mode
```
sudo ./rgb_test --power creator


### RGB Demos
These are work in progresses, re-writing Razer synapse's effects in C++. Currently, they are limited to a demo only (10 seconds).

Starlight mode demo:
```
sudo ./rgb_test --rgb_demo starlight
```

Wave demo (All 4 directions)
```
sudo ./rgb_test --rgb_demo wave
```



