# NOTE:
I forgot to upload my last particle sim, failed hardrive lead to missing works. So I must completely restart
This is a complete work in progress and will be appended to, if not completely modified after the inital simulation is completed

# What
This is an N-body particle sim that will be using cpp and native webgpu bindings.

# Why
The last particle sim I implemented was single threaded and of a single type of particle, I am planning on using this as a base for a fluid simulation for a lava lamp sim.
I choose webgpu in particular, so that I may reuse that code to export the lava lamp simulation to the interwebs. I can then use that lava lamp as a background/screen saver.

# How
In my personal code I prefer to stay away from OOP, so I will mainly be doing procedural programming.
I will be trying to implement concepts that I have been reading in Data-Oriented Design by R.Fabian.
Since I won't be using OOP, and I am comfortable with raw pointers, I will probably be using only std C libs.

## Why not OOP
I like my data and functions being seperate. Literly everything about OOP can be done with procedural programming with less work.
No need for inheritence if you can use the same function on multiple structs.
What is the real difference between polymorphism and void* parameters
Encapsulation, well there are two types in my eyes, data encapsulation, which I don't like, my data will mutate either way, and function encapsulation which cpp and c uses seperate implementation files.

## Why CPP
I like some features of cpp like operator overloading and error handling, and I believe there are real advantages of OOP in correct places


# TODO:

1.  []AABB trees
2.  []Particle single threading
3.  []Particle multithreading
4.  []Webgpu test
5.  []Webgpu particle display
6.  []Port particle physics to webgpu compute shader
7.  []Heat??
8.  []Density??
9.  []Surface tension??
10. []Multiple particles for different fluids




# MAKE IT RUN
One must make a build folder for and run cmake ../
