/**
 * @file tgPlyViewer.cpp
 * @author Thomas Mörwald
 * @date Januar 2014
 * @version 4.0
 * @brief TomGine ply viewer
 */

#include "v4r/TomGine/tgTomGineThread.h"
#include "v4r/TomGine/tgModelLoader.h"


using namespace TomGine;

tgTomGineThread viewer(800,600,"TomGine PLY Viewer", true);

int main (int argc, char *argv[])
{
  std::string ply_file;

  if(argc<2)
  {
    printf("  Usage: tgPlyViewer ply-file\n");
    exit(0);
  }
  else
  {
    ply_file = argv[1];
  }

  tgRenderModel mesh;
  tgModelLoader::LoadPly(mesh, ply_file.c_str());
  //  mesh.m_coloring = TomGine::tgModel::PER_VERTEX_COLORING;

  mesh.ComputeNormals();
  mesh.ComputeBoundingSphere();

  viewer.SetRotationCenter(mesh.m_bs.center);
  viewer.SetInputSpeeds(1.0f, mesh.m_bs.radius, mesh.m_bs.radius);
  viewer.LookAt(mesh.m_bs.center);

  viewer.AddModel3D(mesh);
  viewer.Update();

  printf("  loaded file '%s'\n", ply_file.c_str());
  printf("  faces: %lu  vertices: %lu\n", mesh.m_faces.size(), mesh.m_vertices.size());
  printf("  cor: %f %f %f  radius: %f \n",
         mesh.m_bs.center.x, mesh.m_bs.center.y, mesh.m_bs.center.z, mesh.m_bs.radius);

  viewer.WaitForEvent(TomGine::TMGL_Press, TomGine::TMGL_Escape);
}
