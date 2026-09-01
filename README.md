# Minimal ant agent simulator

A template repository for ant-based simulation. Based on https://github.com/BrainBeatsBrawn/antpov

It's similar to antpov, but is more of a clean-slate template with none of the data provided in that repo.

# Build

## Hardware requirements

You will need an NVIDIA GPU and an Intel/AMD computer (running a Linux OS).

You will need about 13 GB of free storage to install the compilers and build tools and to complete the build process.
My freshly installed and updated system started with 9.5 GB of storage used.
The package managed dependencies added 9 GB; CMake and compound-ray was 2 GB, the antmin repository (and its submodules) another 1 GB and finally the build consumed 1 GB.

## Building on Ubuntu 26.04

You can follow the same instructions as for Ubuntu 24.04, but instead of compiling CMake, you can install with apt:

```bash
sudo apt install cmake
```

## Building on Ubuntu 24.04

Here's how to build and run on an Ubuntu 24.04 system.
It was verified on a cleanly installed system which had been fully upgraded to Ubuntu 24.04.4 LTS.

## Graphics driver

Use the Ubuntu *Additional Driver* GUI to install an NVIDIA graphics driver that is compatible with your NVIDIA GPU.
If you chose to enable "Third party drivers" when you installed Ubuntu, that should have done the job.
You can verify by opening the NVIDIA Settings program and looking at 'System Information'.
You should see an 'NVIDIA Driver Version' field.
If it's not there, then you probably don't have the NVIDIA driver installed.
I've had successful builds on Ubuntu 24 with both the version 535 driver and the version 580 driver.

## Install package managed dependencies

The following packages are required to build antmin:

```bash
sudo apt install build-essential git \
                 gcc-12 g++-12 \
                 nvidia-cuda-toolkit \
                 clang-20 clang-tools-20 libc++-20-dev ninja-build \
                 freeglut3-dev libglu1-mesa-dev libxmu-dev \
                 libxi-dev libglfw3-dev libfreetype-dev libhdf5-dev
```

* GCC 12 and gmake (from build-essential) is used to compile compound-ray.
* Compound-ray also needs nvidia-cuda-toolkit, which installs the NVIDIA GPU compiler nvcc.
* Clang-20 and ninja are used to compile antmin.
* The libraries freeglut3-dev to libhdf5-dev are required by craysim/mathplot for OpenGL visualizations.

## Compile and install cmake

On Ubuntu 24.04, the packaged CMake is slightly too old for the C++ modules build of antmin, so we compile and install the latest CMake from https://cmake.org/download/
At the time of writing this is version 4.3.4.
CMake versions as old as 3.29 and more recent than 4.3.4 should work too, so if your CMake is already within this range, you can skip this step.

```bash
cd ~/Downloads
wget https://github.com/Kitware/CMake/releases/download/v4.3.4/cmake-4.3.4.tar.gz
mkdir -p ~/src
cd ~/src
tar xvf ~/Downloads/cmake-4.3.4.tar.gz
cd cmake-4.3.4
./bootstrap --parallel=4 # If you have 4 cores
make -j4
sudo make install
```

You will now have **/usr/local/bin/cmake** which will be used in preference to the package-managed cmake in /usr/bin. Verify *in a new terminal*:

```bash
seb@ubu24-vm1:~$ which cmake
/usr/local/bin/cmake
seb@ubu24-vm1:~$ cmake --version
cmake version 4.3.4

CMake suite maintained and supported by Kitware (kitware.com/cmake).
seb@ubu24-vm1:~$
```

## Build compound-ray

Compound-ray is a library of code that performs ray casting to compute the colour detected by each ommatidium in an insect compound eye within a model environment.

It uses NVIDIA OptiX to achieve the ray casting on an NVIDIA GPU.

Compound-ray was written by Blayze Millward ([original code](https://github.com/BrainsOnBoard/compound-ray)). A modified version is used here: https://github.com/BrainBeatsBrawn/compound-ray

### NVIDIA OptiX SDK

You will need to register a developer account with NVIDIA and download the [NVIDIA OptiX](https://developer.nvidia.com/rtx/ray-tracing/optix) SDK, version 8.0 from the [Legacy Downloads page](https://developer.nvidia.com/designworks/optix/downloads/legacy).

After downloading, you should have obtained the installer shellscript file **NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64.sh**. We'll assume it's in ~/Downloads/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64.sh.

Unpack the file into ~/src:

```bash
mkdir -p ~/src
cd src
bash ~/Downloads/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64.sh
```

Page down to accept the licence agreement, and then you should be prompted to install in the default location in src/.
Accept the default.
You should now have a directory **~/src/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64** containing the NVIDIA OptiX SDK.

### Temporarily switch your default compiler to GCC 12

With some build systems, you can specify the compiler version with the environment variables `CC` and `CXX`. For some reason, this does not work here, and instead we have to use the `update-alternatives` system (a Debian/Ubuntu thing) to switch our default compiler from GCC 13 to the OptiX 8.0-compatible GCC 12.

```bash
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 12 --slave /usr/bin/g++ g++ /usr/bin/g++-12 --slave /usr/bin/gcov gcov /usr/bin/gcov-12
```

Verify that g++ is version 12:

```bash
seb@ubu24-vm1:~$ g++ --version
g++ (Ubuntu 12.4.0-2ubuntu1~24.04.1) 12.4.0
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

### Obtain and compile compound-ray

```bash
cd ~/src
git clone https://github.com/BrainBeatsBrawn/compound-ray
cd compound-ray/build
cmake .. -DOptiX_INSTALL_DIR=~/src/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64
make
sudo make install # Installs in /usr/local
```

## Build antmin


To compile antmin, clone it then init/update the submodules.

Antmin uses the submodules [sebsjames/mathplot](https://github.com/sebsjames/mathplot), [sebsjames/maths](https://github.com/sebsjames/maths), [BrainBeatsBrawn/craysim](https://github.com/BrainBeatsBrawn/craysim), [BrainBeatsBrawn/oces_viewer](https://github.com/BrainBeatsBrawn/oces_viewer) and [tinygltf](https://github.com/sebsjames/tinygltf).
The program links to Seb's fork of [compound-ray](https://github.com/BrainBeatsBrawn/compound-ray)


```bash
cd ~/src
git clone https://github.com/BrainBeatsBrawn/antmin
cd antmin
git submodule init
git submodule update
```

Now you do a CMake style build, passing the variable `OptiX_INSTALL_DIR`
just as you did for compound-ray and specifying that clang++-20 and ninja should be used to compile:

```bash
mkdir build
cd build
CC=clang-20 CXX=clang++-20 cmake .. -GNinja -DOptiX_INSTALL_DIR=~/src/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64
ninja
```

That's it. Test by launching antmin with a bundled test environment:
```bash
cd ~/src/antmin
./build/antmin -f ./data/natural_env.gltf
```

### Switch your compiler back

Optionally, change your system back, so that the gcc and g++ commands involke the OS-default GCC 13.
You can do this by adding another alternative for GCC 13 or you can simply delete the gcc alternative like this:

```bash
sudo update-alternatives --remove-all gcc
```

# Use the program

## Experimenting with the example environment

Start by familiarising yourself with the capabilities of the software.

```bash
cd ~/src/antmin
./build/antmin -f ./data/natural_env.gltf
```

Three windows should open. One shows the scene, with the ant to be found somewhere within the scene.
Another shows a visualization of the ant's head and eyes, separate from the scene.
The third windows shows a two dimensional representation of the ant's view.

Arrange the windows so that you can see them all, and highlight the one entitled 'Scene'.
This window processes keyboard-input.

Let's find our ant. You can turn on a set of coordinate arrows that are centred on the ant by pressing 'c'.
Zoom out with your scroll wheel to help find the ant coordinate arrows.
You can move your view of the scene with mouse movements and either your left or right button held down.
The left button rotates the scene; the right button translates it.
If you place the agent coordinate arrows at the centre of the window, your rotations should occur around the ant's location.
Zoom in on the coordinate arrows, then switch them off.
You should see our ant, with her compound eyes rendering her view.

You can move her around with some key bindings.
The usual gaming 'wasd' keys move her forwards/backwards, left and right. Change her yaw and roll with the arrow keys. ',' and '.' control her roll.

A summary of all the key bindings is available if you press Ctrl-h (you will see the output in the terminal from which you started the program).

## Make your own Insect POV

The antmin program, with its ant- and application-specific functionality is only a few hundred lines of code.
All the functionality that allows you to place compound-ray eyes in a glTF-encoded scene, and to move the eyes over the surface of a landscape in that scene is held in (mostly C++ modular) libraries.
[Compound-ray](https://github.com/BrainBeatsBrawn/compound-ray) provides the ray casting; [mathplot](https://github.com/sebsjames/mathplot) does OpenGL visualization; [craysim](https://github.com/BrainBeatsBrawn/craysim) binds it all together into a simulation.
The idea is to make it easy to create new simulations.
A new simulation might feature different eye models, different behaviour and it might contain a brain model to control the agent's movement.

You can see the simplest possible craysim program here: [craysim_minimal](https://github.com/BrainBeatsBrawn/craysim_minimal).

This repository extends craysim_minimal with ant-specific features.

# Credits

Antmin was authored by Seb James.
Compound-ray was authored by Blayze Millward, with modifications by Seb James.
Alex Blenkinsop contributed to sebsjames/maths and to craysim.
