/**
 * @file main_tomgine.cpp
 * @author Thomas Mörwald
 * @date October 2009
 * @version 0.1
 * @brief TomGine rendering demo
 */

#include <stdio.h>

#include "v4r/TomGine/tgTomGine.h"
#include <opencv2/highgui/highgui.hpp>

#include <time.h>

using namespace std;
using namespace TomGine;

class myModel: public tgRenderModel {
public:
	myModel() {
		// NO OpenGL commands here (object is constructed in main thread)
	}
	~myModel() {
		// NO OpenGL commands here (object is destroyed in main thread)
	}

	virtual void Draw() {
		// OpenGL commands are allowed here

		//		m_material.Activate();

		tgRenderModel::DrawFaces();

		//glColor3f(0.0f, 0.2f, 0.8f);
		//m_pose.Activate();
		//tgModel::DrawNormals(0.1f);
		//m_pose.Deactivate();
	}
};

int main(int argc, char *argv[]) 
{
    // NO OpenGL commands in main thread!

    unsigned width = 800;
    unsigned height = 600;

    printf("\n Demo TomGineWrapper\n\n");

    printf(" TomGine control\n");
    printf(" -------------------------------------------\n");
    printf(" [Left Mouse Button] Rotate\n");
    printf(" [Right Mouse Button] Move\n");
    printf(" [Scroll Wheel] Zoom\n");
    printf(" [w] Switch to wireframe mode\n");
    printf(" [f] Switch to flat/smooth shading\n");
    printf("\n");
    printf(" TomGineThread control\n");
    printf(" -------------------------------------------\n");
    printf(" [c] Clear scene.\n");
    printf(" [i] Draw background image.\n");
    printf(" [p] Draw point-cloud.\n");
    printf(" [l] Draw labels.\n");
    printf(" [m] Draw models.\n");
    printf(" [n] Draw NURBS objects.\n");
    printf(" \n\n");

    //std::string path_smiley = std::string(TOMGINE_RESOURCES) + "smiley.jpg";
    //cv::Mat cv_img = cv::imread(path_smiley.c_str());

    tgTomGineThread tgThread(width, height);
    tgTimer timer;

    // Attention: OpenGL Calls (like tgModel::DrawFaces() wont work here, since there is no OpenGL context in this thread
    tgRenderModel model;
    model.m_pose.Translate(0.5, 0.0, 0.0);
    tgShapeCreator::CreateSphere(model, 0.5, 2, tgShapeCreator::ICOSAHEDRON);
    tgThread.AddModel(&model);

    //TomGine::tgModel cylinder;
    //tgShapeCreator::CreateCylinder(cylinder, 0.05f, 0.1f, 16, 1, true);
    //tgThread.AddModel(cylinder);

    //tgThread.SetImage(cv_img);

    //tgThread.AddLabel2D("TomGine", 20, 10, 10);
    //tgThread.AddLabel3D("0", 10, vec3(0, 0, 0));

    tgThread.Update();

    tgThread.WaitForEvent(TomGine::TMGL_Press, TomGine::TMGL_Escape);
    return 0;
}

