/**
 * @file main_tomgine.cpp
 * @author Thomas Mörwald
 * @date October 2009
 * @version 0.1
 * @brief TomGine rendering demo
 */

#include <stdio.h>

#include "v4r/TomGine/tgEngine.h"
#include "v4r/TomGine/tgRenderModel.h"
#include "v4r/TomGine/tgModelLoader.h"
#include "v4r/TomGine/tgShapeCreator.h"
#include "v4r/TomGine/tgCollission.h"
#include "v4r/TomGine/tgFont.h"

#include <time.h>

using namespace TomGine;
using namespace std;

typedef vector<vec3> PointList;

int
main (int argc, char *argv[])
{
  printf ("\n Demo TomGine\n\n");

  printf (" TomGine control\n");
  printf (" -------------------------------------------\n");
  printf (" [Left Mouse Button] Rotate\n");
  printf (" [Right Mouse Button] Move\n");
  printf (" [Scroll Wheel] Zoom\n");
  printf (" [w] Switch to wireframe mode\n");
  printf (" [f] Switch to flat/smooth shading\n");
  printf (" [Escape] Quit demo\n");
  printf (" \n\n");

  unsigned width (800);
  unsigned height (600);
  float far (5.0f);
  float near (0.1f);
  std::string name ("TomGine Render Engine");
  bool bfc (true);
  bool threaded (false);
  bool stereo (false);

  tgEngine render (width, height, far, near, name.c_str (), bfc, threaded, stereo);

  srand (time (NULL));
  float fTime;
  tgTimer timer;
  PointList m_points;
  vec3 v;

  // Load Model
  // for more materials visit: http://wiki.delphigl.com/index.php/Materialsammlung
  tgMaterial matSilver;
  matSilver.ambient = vec4 (0.19f, 0.19f, 0.19f, 1.0f);
  matSilver.diffuse = vec4 (0.51f, 0.51f, 0.51f, 1.0f);
  matSilver.specular = vec4 (0.77f, 0.77f, 0.77f, 1.0f);
  matSilver.shininess = 51.2f;

  tgMaterial matRed;
  matRed.Color (1.0f, 0.0f, 0.0f);

  tgMaterial matBlueBlend;
  matBlueBlend.Color (0.0, 0.0, 0.1, 0.5,
                      0.0, 0.0, 0.8, 0.5,
                      0.0, 0.0, 0.2, 0.5,
                      30);

  tgRenderModel shape;
  tgShapeCreator::CreateSphere (shape, 0.05, 3, tgShapeCreator::ICOSAHEDRON);
  //    tgShapeCreator::CreateBox(shape, 0.1,0.1,0.1);
  tgShapeCreator::CreateCylinder (shape, 0.1f, 0.2f, 64, 2, true);
  shape.m_material = matBlueBlend;

  tgRenderModel poly;
  std::vector<vec3> pointlist;
  pointlist.push_back (vec3 (0.1f, 0.01f, -0.1f));
  pointlist.push_back (vec3 (0.00f, 0.0, -0.03f));
  pointlist.push_back (vec3 (-0.1f, 0.01f, -0.1f));
  pointlist.push_back (vec3 (0.0f, 0.01f, 0.2f));
  tgShapeCreator::PolygonMesh (poly, pointlist);
  poly.ComputeFaceNormals ();

  tgRay ray;
  ray.start = render.GetCameraPosition ();
  ray.dir = vec3 (0.01f, 0.0f, 0.0f) - (render.GetCameraPosition () * 2.0f);

  vec3 p;
  std::vector<vec3> pl;
  std::vector<vec3> nl;
  std::vector<double> zl;

  //  bool b = tgCollission::IntersectRayTriangle(p, z, ray, pointlist[0], pointlist[1], pointlist[2]);
  bool b = tgCollission::IntersectRayModel (pl, nl, zl, ray, shape);

  // Rendering loop
  while (render.ProcessEvents ())
  {
    fTime = timer.Update ();
    render.Activate3D ();

    shape.DrawFaces ();

    poly.DrawFaces ();

    glDisable (GL_LIGHTING);
    glBegin (GL_LINES);
    glVertex3f (ray.start.x, ray.start.y, ray.start.z);
    glVertex3f (ray.start.x + ray.dir.x, ray.start.y + ray.dir.y, ray.start.z + ray.dir.z);
    glEnd ();
    glDisable (GL_TEXTURE_2D);

    for (unsigned int i = 0; i < pl.size (); i++)
    {
      glDisable (GL_DEPTH_TEST);
      glPointSize (3);
      glBegin (GL_POINTS);
      glColor3f (1.0f - b, b, 0.0f);
      glVertex3f (pl[i].x, pl[i].y, pl[i].z);
      glEnd ();
      glEnable (GL_DEPTH_TEST);
    }
    glEnable (GL_LIGHTING);

    render.DrawCoordinates (1.0, 1.0);

    render.Activate2D ();
    //g_font->Print ("TomGine Render Engine", 24, 7, 7);

    //render.PrintText ("TomGine Render Engine", 7, 7);
    render.Update (fTime);
  }

  return 0;
}

