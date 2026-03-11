# NOTE:
I forgot to upload my last particle sim, failed hardrive lead to missing works. So I must completely restart
This is a complete work in progress and will be appended to, if not completely modified after the inital simulation is completed

# What
This is an N-body particle sim that will be using cpp and native webgpu bindings.

# Why
The last particle sim I implemented was single threaded and of a single type of particle, I am planning on using this as a base for a fluid simulation for a lava lamp sim.
I choose webgpu in particular, so that I may reuse that code to export the lava lamp simulation to the interwebs. I can then use that lava lamp as a background/screen saver.


# TODO:

1.  [x]AABB trees
2.  [x]Particle single threading
3.  [x]Particle multithreading
4.  [x]Webgpu particle display
5.  [x]Port particle physics to webgpu compute shader
6.  []Heat??
7.  []Density??
8.  []Surface tension??
9. []Multiple particles for different fluids



# MAKE IT RUN

## Building

```bash
git clone --depth 1 https://github.com/google/dawn.git  # if dawn/ dir is missing
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

First build is slow because of Dawn compilation. Subsequent builds are fast.


## Usage

```bash
./ParticleSim
```
