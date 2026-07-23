# cs488

# Compilation 
No additional measures need to be taken to compile and run the project. 

# Specification
For task 1: I used Verlet integration to update the positition and the velocity. The new velocity is computed 
as (newPosition - oldPosition) / deltaT. 

For task 2: I used the position based approach where I reflected the old position about the axis at the new 
position and moved them both according to where the wall is (in this case the bounding box). I also added a
restitution factor of 0.8 so that the bounce is more realistic. 

For task 3: I projected on the surface of the sphere centered by computing the distance to the center
of the sphere and following the position-based approach just like the projection for motion on a bead. 
The function resolveSphereConstraint() handles this. 

For task 4: I have a gravitationalForces array in ParticleSystem that keeps track of the accumulated 
forces on each particle due to gravity from other particles. I used (2e - 3f) for the value of 
graviation constant times the masses of the particles. The function computeGravitationalField()
loops over all the particles, and then again to account for all the forces. I added some small offset 
to the distance calculation so that the distance cubed doesn't cause the force to explode. 

For task 5: I used simulatenous collision detection and iterated 5 times to resolve sphere collisions. 
I also multiplied the collision response by the collision normal so that the spheres move away in the 
right direction. I also resolve the surface sphere constraint right after so that particles are always 
on the surface of the sphere as in task 3. 


