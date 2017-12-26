# N-Body-Simulation
This is an implementation of N Body simulation. 

An N-body simulation approximates the motion of particles, often specifically particles that interact with one another through some type of physical forces. Using this broad definition, the types of particles that can be simulated using n-body methods are quite significant, ranging from celestial bodies to individual atoms in a gas cloud. From here out, we will specialize the conversation to gravitational interactions, where individual particles are defined as a physical celestial body, such as a planet, star, or black hole. Motion of the particles on the bodies themselves is neglected, since it is often not interesting to the problem and will hence add an unnecessarily large number of particles to the simulation. You can more about N-Body simulation from: http://physics.princeton.edu/~fpretori/Nbody/intro.htm  


To start serial_N_body_simulation.c, you have enter following commands: 
1) gcc serial_N_body_simulation.c -o serial -lm
2) ./serial 0.1 100  
// 0.1 is the delta time, by default it is 0.1
// 100 is the number of bodies, default value is 100

To run parallel_N_body_simulation.c, enter:
1)  mpicc parallel_N_body_simulation.c -o parallel -lm
2)  mpiexec -n 10 ./parallel 0.1 100 // 10 is the number of processes 
