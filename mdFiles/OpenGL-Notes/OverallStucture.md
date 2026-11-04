# Overall shader pipeline 

## 1. Vertex Shader 
This shader transforms each vertex into clip space coordinate (gl_Position)  
Model, View, Projection matrix multiply happens here

## 2. Geometry Shader 
**This step is optional**  
Takes a primitive(points, Line, Triangle) and can add vertices or make a different primitive  
**Not in GLES 3.0** so don't use it in shared code

## 3. Shape assembly 
make a actual triangle via vertices  
the primitive type of the draw call (GL_TRIANGLES etc.) decides the shape  
EBO(Element Buffer Object) is the list of indices that tells which vertices to use and in what order  
btw EBO is optional but it is very important for shrink the size of the Vertex Data  
ex) rectangle = 4 vertices + 6 indices (not 6 vertices)

## 4. Rasterization 
Finds the fragments(pixel candidates) that the primitive covers  
Also interpolates the vertex shader outputs (color, UV...) between the vertices  
how to fill can be determined by glPolygonMode (fill, line, point)  
**glPolygonMode is not in GLES** so use #ifndef USE_GLES

## 5. Fragment Shader 
This shader colors the fragments that was Rasterizated  

## 6. Blending  
Mix the new fragment color with the color already in the framebuffer  
Needs draw order, transparent objects should be drawn far → near so that it prints on screen correctly
