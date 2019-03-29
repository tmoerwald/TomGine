
#include "v4r/TomGine/tgModelLoader.h"



int main(int argc, char * argv[])
{
  std::string in_file;
  std::string out_file;

  if (argc > 2)
  {
    in_file = argv[1];
    out_file = argv[2];
  } else
  {
    printf("Usage: tomgine_remove_dublicates ply-file-in ply-file-out\n\n");
    return 0;
  }


  // load file
  TomGine::tgModel model;
  TomGine::tgModelLoader::LoadPly(model, in_file.c_str());

  // remove dublicate vertices
  model.RemoveDublicateVertices(false);

  // save file
  TomGine::tgModelLoader::SavePly(model, out_file.c_str(), false);

  return 0;
}
