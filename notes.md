## Default Shader
Default shader is a class that every other shader should extend on.

It has some utilities to build shader as little repetition as possible.
 * initDefaultBuffers initializes all the default buffers
 * addSampler adds a default sampler 
 * addDynamicBuffer creates a buffer of type <T>
 * map/unmapBuffersVS/PS helper functions to map/unmap buffers of type <T>

The attenuation factor is hard coded using values from Ogre3D, every
light has a "lightStrength" value that goes from 0 to 11. Lower values
mean the light won't travel as much.
The default shader also passes some data to the vertex shader that might not
be useful to every shader, but help generalize the class.
The default shader has a simpler render function that only accepts a MMesh object,
a MMesh is just a simple BaseMesh with an (optional) texture and diffuse color.
A MMesh object can be initialized with temporary data and it can be cleared using
the clear method. It can also move data from a BaseMesh to itself and delete it 
when the destructor is called. This lets you use any class that extends a BaseMesh in a MMesh.







### notes
- [ ] maybe use a class hierarchy graph 
- pcf could be applied to the point light's shadows, but it would hurt performance
  and it is very hard to notice the shadow quality thanks to the grass
- modified BaseShader.h so you could load a vertex shader and pass the layout.
  usefeful for instancing