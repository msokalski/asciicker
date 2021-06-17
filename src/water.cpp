 

// we want ability of having super-lo-res mesh scattered over terrain (please: no more than 10 triangles in viewport)
// in similar way we'd like to have object meshes (but instanced)

// 1. render terrain AND object instances
// 2. depth-test water check if any water triangle cell is visible, link visible water triangles into temp-list
// 3. for each visible water triangle:
//    - calc reflection transform, planes bounding triangle, bbox planes bounding visible area
//    - gather and render reflection of terrain patches and object instances both clipped by these planes

// looks like we should start with objects before adding water!
// we need OBJ importer, pure mesh with single UV channel (no normals, colors, smooth groups etc)


/*
CASCADED REFLECTION:

given reflection plane:  A*x+B*y+C*z+D==0
transformed P=[x,y,z,1] is: [x,y,-2*(A*x+B*y+D)/C-z,1]

so matrix is (if needed at all):
[  1     0     0     0  ]
[  0     1     0     0  ]
[-2A/C -2B/C  -1   -2D/C]
[  0     0     0     1  ]

what clipping planes should we use to query geomtery?
- reflection of 4x viewport-edge planes
- reflection of planes constructed from viewing vector and mirror boundary edges
- mirror plane (reflection is same)


note: sprites can be deformed!
      if reflection plane's normal in view coords has non zero X coord
	  (water flow is somewhat horizontal on screen)
	  we'd need to render sprites z-column by z-column adjusting
	  everytime column's screen space Y position
*/

// HOW DO WE STORE POINTS, TRIANGLES, POLYGONS, PLANES ?
/*

// we will need fast queries:
// by ray -> get xyz and closest triangle
// by clipping planes -> callback with triangle

*/
