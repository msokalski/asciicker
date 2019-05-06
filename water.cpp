

// we want ability of having super-lo-res mesh scattered over terrain (please: no more than 10 triangles in viewport)
// in similar way we'd like to have object meshes (but instanced)

// 1. render terrain AND object instances
// 2. depth-test water check if any water triangle cell is visible, link visible water triangles into temp-list
// 3. for each visible water triangle:
//    - calc reflection transform, planes bounding triangle, bbox planes bounding visible area
//    - gather and render reflection of terrain patches and object instances both clipped by these planes

// looks like we should start with objects before adding water!
// we need OBJ importer, pure mesh with single UV channel (no normals, colors, smooth groups etc)
