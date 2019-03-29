/*
 * Software License Agreement (GNU General Public License)
 *
 *  Copyright (c) 2011, Thomas Mörwald
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author thomas.moerwald
 *
 */
#include "tgTomGineThread.h"
#include <iomanip>

using namespace TomGine;

/** ThreadDrawing (OpenGL commands are only allowed in this thread) */
namespace TomGine 
{
    void ThreadDrawing(void* c)
    {
      tgTomGineThread *tt = (tgTomGineThread*) c;

      TomGine::tgEngine* render = new TomGine::tgEngine(tt->m_width, tt->m_height, tt->m_depth_max, tt->m_depth_min,
                                                        tt->m_windowname.c_str(), tt->m_bfc, true);
      render->Update();

      tt-> dataMutex.Lock();
      tt->m_engine = render;
      bool stop = tt->m_stopTomGineThread;
      tt->dataMutex.Unlock();

      std::list<Event> eventlist;
      float fTime;
      unsigned fps;

      while (!stop)
      {
        //if (!tt->m_FPS)
        //  tt->renderSem.Wait();
          Event event;
          tt->m_engine->WaitForEvent(event);
          if (tt->KeyHandler(event))
              tt->m_stopTomGineThread = true;

          tt->eventMutex.Lock();
          if (tt->m_waitingForEvents)
          {
              if (event.type == TMGL_Press || event.type == TMGL_Release)
              {
                  tt->m_waitingeventlist.push_back(event);
              }
          }
          tt->eventMutex.Unlock();

        tt->dataMutex.Lock();
        if (tt->m_clearcommand != tt->GL_CLEAR_NONE)
        {
          tt->GL_Clear();
          tt->clearFinishedSem.Post();
        }
        tt->dataMutex.Unlock();

        //tt->eventMutex.Lock();
        //eventlist.clear();
        //while (!tt->m_eventlist.empty())
        //{
        //  eventlist.push_back(tt->m_eventlist.front());
        //  tt->m_eventlist.pop_front();
        //  tt->renderSem.TryWait();
        //}
        //tt->m_eventlist.clear();
        //tt->eventMutex.Unlock();

        tt->dataMutex.Lock();
        //while (!eventlist.empty())
        //{
        //    tt->m_engine->InputControl(eventlist.front());
        //  eventlist.pop_front();
        //}

        tt->GL_Update(tt->m_engine);
        tt->GL_Draw(tt->m_engine);

        stop = tt->m_stopTomGineThread;
        tt->dataMutex.Unlock();

        if (tt->m_waitForZBuffer)
        {
          tt->dataMutex.Lock();
          tt->GL_ZBuffer(tt->m_zBuffer, tt->m_engine);
          tt->m_waitForZBuffer = false;
          tt->dataMutex.Unlock();
        }
        if (tt->m_snapshot)
        {
          tt->dataMutex.Lock();
          tt->GL_Snapshot(tt->m_snapshotFile);
          tt->m_snapshot = false;
          tt->dataMutex.Unlock();
        }
        if (tt->m_record)
        {
          tt->dataMutex.Lock();
          tt->GL_Record(tt->m_engine);
          tt->dataMutex.Unlock();
        }

        tt->m_engine->Update(fTime);

        if (tt->m_FPS)
        {
          fps = unsigned(round(1.0 / fTime));
          tt->dataMutex.Lock();
          if (fps > tt->m_maxFPS)
            tt->m_fTime += 0.001;
          else if (fps < tt->m_maxFPS)
            tt->m_fTime -= 0.001;
          tt->m_fTime < 0.0 ? fTime = -tt->m_fTime : fTime = tt->m_fTime;
          tt->dataMutex.Unlock();
      
          Sleep(1);
          //usleep(unsigned(fTime * 1e6));
        }

        tt->renderFinishedSem.Post();

      }

      tt->dataMutex.Lock();
      tt->GL_Clear();
      tt->m_engine = NULL;
      tt->dataMutex.Unlock();

      render->UnWaitForEvent();
      delete render;

      tt->dataMutex.Lock();
      tt->m_renderingStopped = true;
      tt->dataMutex.Unlock();

      tt->renderFinishedSem.Post();

      //pthread_exit(NULL);
    }

    void ThreadEventHandling(void* c)
    {
      tgTomGineThread *tt = (tgTomGineThread*) c;

      tt->dataMutex.Lock();
      bool stop = tt->m_stopTomGineThread;
      tt->dataMutex.Unlock();

      bool engineExists = false;

      Event event;
      while (!stop)
      {
        if (engineExists)
        {
          tt->m_engine->WaitForEvent(event);

          tt->dataMutex.Lock();
          //if (tt->KeyHandler(event))
          //  tt->m_stopTomGineThread = true;
          tt->m_engine->ProcessEvents();
          tt->dataMutex.Unlock();

          if(tt->m_eventCallback)
            tt->m_eventListner->EventFunction(event);

          tt->eventMutex.Lock();
          tt->m_eventlist.push_back(event);

          if (tt->m_waitingForEvents)
            if (event.type == TMGL_Press || event.type == TMGL_Release)
            {
              tt->m_waitingeventlist.push_back(event);
            }

          if (tt->m_listenForEvents)
          {
            tt->m_listenEventList.push_front(event);
            if (tt->m_listenEventList.size() > tt->m_listenEventListSize) // avoid out-of-memory
              tt->m_listenEventList.resize(tt->m_listenEventListSize);
          }
          tt->Update();
          tt->eventMutex.Unlock();

        } else
        {
          //usleep(10000); // wait for render engine to start up
          Sleep(10);
        }
        tt->dataMutex.Lock();
        stop = tt->m_stopTomGineThread;
        engineExists = tt->m_engine;
        tt->dataMutex.Unlock();
      }

      tt->dataMutex.Lock();
      tt->m_eventsStopped = true;
      tt->dataMutex.Unlock();

      //pthread_exit(NULL);
    }
}

/********************** tgTomGineThread ************************/
void tgTomGineThread::init()
{
  m_stopTomGineThread = false;
  m_renderingStopped = false;
  m_eventsStopped = false;
  m_clearcommand = GL_CLEAR_NONE;
  m_clearindex = 0;
  m_clearColor = TomGine::vec3(0.0f, 0.0f, 0.0f);
  m_maxFPS = 0;
  m_FPS = false;
  m_fTime = 0.0;
  m_snapshot = false;
  m_record = false;
  m_recorderFrame = 0;
  m_numScreenShot = 0;

  df.image = true;
  df.pointclouds = true;
  df.cameras = true;
  df.lines = true;
  df.points = true;
  df.labels = true;
  df.models = true;
  df.normals = false;
  df.textured = true;

  m_loadImage = false;
  m_showCoordinateFrame = false;
  m_camChanged = false;
//  m_tgCamChanged = false;
  m_inputSpeedChanged = false;
  m_rotCenterChanged = false;
  m_waitingForEvents = false;
  m_listenForEvents = false;
  m_waitForZBuffer = false;

  m_listenEventListSize = 100;
  m_eventCallback = false;

  m_engine = NULL;
}

tgTomGineThread::tgTomGineThread(int w, int h, std::string windowname, bool bfc, float depth_min, float depth_max) :
  m_width(w), m_height(h), m_depth_min(depth_min), m_depth_max(depth_max), m_bfc(bfc), m_windowname(windowname),
  m_normal_length(0.1), thread_gl(ThreadDrawing, this) // , thread_event(ThreadEventHandling, this)
{
    init();
    Update();
}

tgTomGineThread::~tgTomGineThread()
{
  m_stopTomGineThread = true;
  Update();
  renderFinishedSem.Post();
  clearFinishedSem.Post();
  snapshotSem.Post();

  thread_gl.Join();
  //thread_event.Join();
}

void tgTomGineThread::GL_Clear()
{
  const ClearCommand &cc = m_clearcommand;

  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_POINTCLOUDS)
  {
    for (unsigned i = m_clearindex; i < this->m_pointclouds.size(); i++)
      if (m_pointclouds[i] != NULL)
        delete m_pointclouds[i];
    m_pointclouds.resize(m_clearindex);
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_CAMERAS)
    m_cameras.clear();
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_POINTS_2D)
  {
    m_points2D.clear();
    m_pointSize2D.clear();
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_POINTS_3D)
  {
    m_points3D.clear();
    m_pointSize3D.clear();
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_LINES_2D)
  {
    m_lines2D.clear();
    m_lineCols2D.clear();
    m_lineWidth2D.clear();
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_LINES_3D)
  {
    m_lines3D.clear();
    m_lineCols3D.clear();
    m_lineWidth3D.clear();
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_LABELS)
  {
    m_labels2D.clear();
    m_labels3D.clear();
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_MODELS_2D)
  {
    for (size_t i = m_clearindex; i < m_models2D.size(); i++)
      if (m_models2D[i] != NULL)
        delete m_models2D[i];
    m_models2D.resize(m_clearindex);
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_MODELS_3D)
  {
    for (size_t i = m_clearindex; i < m_models3D.size(); i++)
      if (m_models3D[i] != NULL)
        delete m_models3D[i];
    m_models3D.resize(m_clearindex);
  }
  if (cc == GL_CLEAR_ALL || cc == GL_CLEAR_MODEL_POINTERS)
  {
    m_modelpointers.clear();
  }
  m_clearindex = 0;
}

void tgTomGineThread::PrintUsage()
{
  printf("\n\n --- TomGine Render Engine ---\n\n");
  printf("  [Escape,q] Quit\n");
  printf("  [f] Show / Hide coordinate frame\n");
  printf("  [d] Show / Hide points (dots)\n");
  printf("  [i] Show / Hide background image\n");
  printf("  [p] Show / Hide point-clouds\n");
  printf("  [t] Show / Hide labels (text)\n");
  printf("  [l] Show / Hide lines\n");
  printf("  [m] Show / Hide models\n");
  printf("  [o] Print camera info\n");
  printf("  [r] Print current center of rotation\n");
  printf("  [F11] Take screenshot\n");
  printf("\n\n");
}

bool tgTomGineThread::KeyHandler(Event &event)
{
  if (event.type == TMGL_Quit)
    return true;
  if (event.type == TMGL_Press)
  {
    if (event.input == TMGL_Escape || event.input == TMGL_q)
      return true;
    //  [clear] internal events should not manipulate data (external processes might need it)
    else if (event.input == TMGL_f)
      m_showCoordinateFrame = !m_showCoordinateFrame;
    else if (event.input == TMGL_c)
      df.textured = !df.textured;
//      df.cameras = !df.cameras;
    else if (event.input == TMGL_d)
      df.points = !df.points;
    else if (event.input == TMGL_i)
      df.image = !df.image;
    else if (event.input == TMGL_p)
      df.pointclouds = !df.pointclouds;
    else if (event.input == TMGL_t)
      df.labels = !df.labels;
    else if (event.input == TMGL_l)
      df.lines = !df.lines;
    else if (event.input == TMGL_m)
      df.models = !df.models;
    else if (event.input == TMGL_n)
      df.normals = !df.normals;
    else if (event.input == TMGL_o)
      m_engine->GetCamera().Print();
    else if (event.input == TMGL_r)
      printf("COR: %f %f %f\n", m_rotCenter.x, m_rotCenter.y, m_rotCenter.z);
    else if (event.input == TMGL_KP_Add)
      for (size_t i = 0; i < m_pointclouds.size(); i++)
        m_pointclouds[i]->m_point_size += 1.0f;
    else if (event.input == TMGL_KP_Subtract)
      for (size_t i = 0; i < m_pointclouds.size(); i++)
      {
        float &ps = m_pointclouds[i]->m_point_size;
        ps <= 1.0f ? ps = 1.0f : ps -= 1.0f;
      }
    else if (event.input == TMGL_F11)
    {
      std::ostringstream os;
      os << "tgImg_" << std::setfill('0') << std::setw(4) << this->m_numScreenShot++ << ".png";
      m_snapshotFile = os.str();
      m_snapshot = true;
    }
  }
  return false;
}

void tgTomGineThread::GL_DrawCameras()
{
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glLineWidth(1.0);
  glColor3f(0.5f, 0.5f, 0.5f);

  for (size_t i = 0; i < m_cameras.size(); i++)
    m_cameras[i].GetFrustum()->DrawFrustum();
}

void tgTomGineThread::GL_DrawPointCloud()
{
  glDisable (GL_POINT_SMOOTH);

  for (unsigned i = 0; i < this->m_pointclouds.size(); i++)
  {
    this->m_pointclouds[i]->DrawColorPoints();
    if (df.normals)
      this->m_pointclouds[i]->DrawLines();
  }

#ifdef DEBUG
  tgCheckError("[tgTomGineThread::DrawPointCloud]");
#endif
}

void tgTomGineThread::GL_DrawPoints3D()
{
  glDisable(GL_LIGHTING);
  glEnable(GL_POINT_SMOOTH);

  for (unsigned i = 0; i < m_points3D.size(); i++)
  {
    glPointSize(m_pointSize3D[i]);
    glBegin(GL_POINTS);
    glColor3ub(m_points3D[i].color[0], m_points3D[i].color[1], m_points3D[i].color[2]);
    glVertex3f(m_points3D[i].pos.x, m_points3D[i].pos.y, m_points3D[i].pos.z);
    glEnd();
  }
  glPointSize(1.0f);

#ifdef DEBUG
  tgCheckError("[tgTomGineThread::DrawPoints3D]");
#endif
}

void tgTomGineThread::GL_DrawPoints2D()
{
  glDisable(GL_LIGHTING);
  //glEnable(GL_POINT_SMOOTH);

  for (unsigned i = 0; i < m_points2D.size(); i++)
  {
    glPointSize(m_pointSize2D[i]);
    glBegin(GL_POINTS);
    glColor3ub(m_points2D[i].color[0], m_points2D[i].color[1], m_points2D[i].color[2]);
    glVertex3f(m_points2D[i].pos.x, m_points2D[i].pos.y, m_points2D[i].pos.z);
    glEnd();
  }
  glPointSize(1.0f);

#ifdef DEBUG
  tgCheckError("[tgTomGineThread::DrawPoints2D]");
#endif
}

void tgTomGineThread::GL_DrawLines2D()
{
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  for (unsigned i = 0; i < m_lines2D.size(); i++)
  {
    glLineWidth(m_lineWidth2D[i]);
    glBegin(GL_LINES);
    glColor3f(m_lineCols2D[i].x, m_lineCols2D[i].y, m_lineCols2D[i].z);
    glVertex3f(m_lines2D[i].start.x, m_lines2D[i].start.y, m_lines2D[i].start.z);
    glVertex3f(m_lines2D[i].end.x, m_lines2D[i].end.y, m_lines2D[i].end.z);
    glEnd();
  }
  glLineWidth(1.0f);
#ifdef DEBUG
  tgCheckError("[tgTomGineThread::DrawLines2D]");
#endif
}

void tgTomGineThread::GL_DrawLines3D()
{
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  for (unsigned i = 0; i < m_lines3D.size(); i++)
  {
    glLineWidth(m_lineWidth3D[i]);
    glBegin(GL_LINES);
    glColor3f(m_lineCols3D[i].x, m_lineCols3D[i].y, m_lineCols3D[i].z);
    glVertex3f(m_lines3D[i].start.x, m_lines3D[i].start.y, m_lines3D[i].start.z);
    glVertex3f(m_lines3D[i].end.x, m_lines3D[i].end.y, m_lines3D[i].end.z);
    glEnd();
  }
  glLineWidth(1.0f);
#ifdef DEBUG
  tgCheckError("[tgTomGineThread::DrawLines3D]");
#endif
}

void tgTomGineThread::GL_DrawLabels(TomGine::tgEngine *render)
{
  render->Activate3D();
  mat4 modelview, projection;
  vec4 viewport;
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
  glGetFloatv(GL_PROJECTION_MATRIX, projection);
  glGetFloatv(GL_VIEWPORT, viewport);
  mat4 modelviewprojection = projection * modelview;

  render->Activate2D();
  for (unsigned i = 0; i < this->m_labels3D.size(); i++)
  {
    tgLabel3D l = m_labels3D[i];
    vec4 texCoords = modelviewprojection * vec4(l.pos.x, l.pos.y, l.pos.z, 1.0);

    if (texCoords.z < 0.0f)
        continue;

    float x = (texCoords.x / texCoords.w + 1.0f) * 0.5f;
    float y = (texCoords.y / texCoords.w + 1.0f) * 0.5f;

    g_font->Print(l.text.c_str(), l.size, int(viewport.z * x), int(viewport.w * y), l.rgba.x, l.rgba.y, l.rgba.z, l.rgba.w);
  }

  for (unsigned i = 0; i < this->m_labels2D.size(); i++)
  {
    tgLabel2D l = m_labels2D[i];
    g_font->Print(l);
  }
}

void tgTomGineThread::GL_DrawModels2D()
{
  for (unsigned i = 0; i < this->m_models2D.size(); i++)
  {
    if (m_models2D[i] != NULL)
      m_models2D[i]->Draw();
  }
#ifdef DEBUG
  tgCheckError("[tgTomGineThread::GL_DrawModels2D]");
#endif
}

void tgTomGineThread::GL_DrawModels3D(bool textured)
{
  for (unsigned i = 0; i < this->m_models3D.size(); i++)
  {
    if (m_models3D[i] != NULL)
    {
      m_models3D[i]->Draw(textured);
      if (df.normals)
        m_models3D[i]->DrawNormals(m_normal_length);
    }
  }
  for (unsigned i = 0; i < this->m_modelpointers.size(); i++)
  {
    if (m_modelpointers[i] != NULL)
    {
      m_modelpointers[i]->Draw();
      if (df.normals)
        m_modelpointers[i]->DrawNormals(m_normal_length);
    }
  }
#ifdef DEBUG
  tgCheckError("[tgTomGineThread::GL_DrawModels3D]");
#endif
}

//void tgTomGineThread::GL_SyncNurbsData()
//{
//  // add new nurbs
//  while (this->m_nurbsSurface.size() < this->m_nurbsSurfaceData.size()) {
//    unsigned id = this->m_nurbsSurface.size();
//    TomGine::tgNurbsSurface *surf = new TomGine::tgNurbsSurface(this->m_nurbsSurfaceData[id],
//        m_engine->m_NurbsSurfaceShader);
//    this->m_nurbsSurface.push_back(surf);
//    this->m_nurbsSurfaceData[id].sync = true;
//  }
//#ifdef DEBUG
//  tgCheckError("[tgTomGineThread::SyncNurbsData] add new nurbs");
//#endif
//  // update out of sync nurbs
//  for (unsigned i = 0; i < this->m_nurbsSurfaceData.size(); i++) {
//    if (!this->m_nurbsSurfaceData[i].sync) {
//      this->m_nurbsSurface[i]->Set(this->m_nurbsSurfaceData[i]);
//      this->m_nurbsSurfaceData[i].sync = true;
//    }
//  }
//
//}
//
//void tgTomGineThread::GL_DrawNurbs()
//{
//  for (unsigned i = 0; i < this->m_nurbsSurface.size(); i++) {
//    glLineWidth(1.0f);
//    this->m_nurbsSurface[i]->DrawFaces();
//    glPointSize(5.0);
//    glColor3f(0.6, 0.0, 0.6);
//    this->m_nurbsSurface[i]->DrawCPs();
//  }
//#ifdef DEBUG
//  tgCheckError("[tgTomGineThread::DrawNurbs]");
//#endif
//}

void tgTomGineThread::GL_Record(TomGine::tgEngine *render)
{
  char img_number[16];
  sprintf(img_number, "img-%06d.jpg", m_recorderFrame++);
  std::string filename = m_recorderPath;
  filename.append(img_number);
  GL_Snapshot(filename);

  render->Activate2D();
  glPointSize(20);
  glBegin(GL_POINTS);
  glColor3ub(255, 0, 0);
  glVertex3f(m_width - 15, m_height - 15, 0.0);
  glEnd();

  render->DrawFPS();
}

void tgTomGineThread::GL_Snapshot(const std::string &filename)
{
  TomGine::tgTexture2D tex;
  tex.CopyTexImage2D(m_width, m_height, GL_RGB);
  tex.Save(filename.c_str());
//  printf("[tgTomGineThread::GL_Snapshot] Saved image to '%s'\n", filename.c_str());
  snapshotSem.Post();
}

void tgTomGineThread::GL_ZBuffer(cv::Mat1f &z, TomGine::tgEngine *render)
{
  z = cv::Mat1f(m_height, m_width);
  if (!z.isContinuous())
    printf("[tgTomGineThread::GL_ZBuffer] Warning, cv::Mat1f is not memory aligned.\n");

  float far = render->GetCamera().GetZFar();
  float near = render->GetCamera().GetZNear();
  float c = (far + near) / (far - near);
  float d = (2 * far * near) / (far - near);

  glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT, GL_FLOAT, &z(0, 0));

  for (int i = 0; i < z.rows; i++)
    for (int j = 0; j < z.cols; j++)
    {
      //      z(i, j) = d / (c - z(i, j));
      float &val = z(i, j);
      val = 2.0f * val - 1.0f;
      val = d / (c - val);
    }
}

void tgTomGineThread::GL_Update(TomGine::tgEngine *render)
{
  glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.);

  // set camera pose...
  if (m_camChanged)
  {
    unsigned w = unsigned(m_width);
    unsigned h = unsigned(m_height);
    render->SetCamera(m_intrinsic, w, h, m_extR, m_extT);
    m_camChanged = false;
#ifdef DEBUG
    tgCheckError("[tgTomGineThread::GL_Update] SetCamera (cv):");
#endif
  }
  //  if (m_tgCamChanged)
  //  {
  //    render->SetCamera(m_tgCamera);
  //    m_tgCamChanged = false;
  //#ifdef DEBUG
  //    tgCheckError("[tgTomGineThread::GL_Update] SetCamera (tg):");
  //#endif
  //  }
  if (m_inputSpeedChanged)
  {
    render->SetSpeeds(m_inputSpeeds.x, m_inputSpeeds.y, m_inputSpeeds.z);
    m_inputSpeedChanged = false;
  }
  // set center of rotation
  if (m_rotCenterChanged)
  {
    render->SetCenterOfRotation(m_rotCenter.x, m_rotCenter.y, m_rotCenter.z);
#ifdef DEBUG
    tgCheckError("[tgTomGineThread::GL_Update]");
#endif
  }
  // synchronize NURBS data
  //  if (df.nurbs)
  //    GL_SyncNurbsData();

}

void tgTomGineThread::GL_Draw(TomGine::tgEngine *render)
{
  if (m_loadImage && !m_img.empty())
  {
    if (m_img.channels() == 1)
      render->LoadBackgroundImage(m_img.data, m_img.cols, m_img.rows, GL_LUMINANCE, true);
    if (m_img.channels() == 3)
      render->LoadBackgroundImage(m_img.data, m_img.cols, m_img.rows, GL_BGR, true);
    m_loadImage = false;
  }

  render->Activate2D();
  if (df.image)
    render->DrawBackgroundImage(NULL, true);

  if (df.points)
    GL_DrawPoints2D();

  if (df.lines)
    GL_DrawLines2D();

  if (df.models)
    GL_DrawModels2D();

  render->Activate3D();

  if (df.models)
    GL_DrawModels3D(!render->GetWireframeMode() && df.textured);

  if (df.pointclouds)
    GL_DrawPointCloud();

  if (df.points)
    GL_DrawPoints3D();

  if (df.lines)
    GL_DrawLines3D();

  if (df.cameras)
    GL_DrawCameras();

  if (m_showCoordinateFrame)
    render->DrawCoordinates(1.0, 2.0);

  if (df.labels)
    GL_DrawLabels(render);

}

/***************************** PUBLIC *****************************/

Event tgTomGineThread::WaitForEvent(Type type)
{
  Event event;
  bool valid = false;
  while (!valid && !this->m_renderingStopped)
  {
    eventMutex.Lock();
    m_waitingForEvents = true;
    eventMutex.Unlock();

    renderFinishedSem.Wait();
    eventMutex.Lock();
    m_waitingForEvents = false;
    while (!m_waitingeventlist.empty())
    {
      event = m_waitingeventlist.front();
      m_waitingeventlist.pop_front();

      if (event.type == type)
        valid = true;
    }
    eventMutex.Unlock();
  }

  if (!valid)
    event.type = TMGL_None;

  return event;
}

Event tgTomGineThread::WaitForEvent(Type type, Input input)
{
  Event event;
  bool valid = false;
  while (!valid && !this->m_renderingStopped)
  {
    eventMutex.Lock();
    m_waitingForEvents = true;
    eventMutex.Unlock();

    renderFinishedSem.Wait();
    eventMutex.Lock();
    m_waitingForEvents = false;
    while (!m_waitingeventlist.empty())
    {
      event = m_waitingeventlist.front();
      m_waitingeventlist.pop_front();

      if (event.type == type)
      {
        if (event.input == input)
        {
          valid = true;
        }
      }

    }
    eventMutex.Unlock();
  }

  if (!valid)
    event.type = TMGL_Quit;

  return event;
}

void tgTomGineThread::StartEventListener(unsigned max_events)
{
  eventMutex.Lock();
  this->m_listenForEvents = true;
  this->m_listenEventListSize = max_events;
  eventMutex.Unlock();
}

void tgTomGineThread::RegisterEventListener(tgEventListener *listener)
{
  eventMutex.Lock();
  this->m_eventListner = listener;
  this->m_eventCallback = true;
  eventMutex.Unlock();
}

void tgTomGineThread::StopEventListener()
{
  eventMutex.Lock();
  this->m_listenForEvents = false;
  eventMutex.Unlock();
}

void tgTomGineThread::GetEventQueue(std::list<Event> &events, bool waiting)
{
  if(waiting && this->m_listenEventList.empty())
    renderFinishedSem.Wait();

  eventMutex.Lock();
  events = this->m_listenEventList;
  this->m_listenEventList.clear();
  eventMutex.Unlock();
}

bool ascending(double i, double j)
{
  return (i < j);
}

bool tgTomGineThread::SelectModels(int x, int y, std::vector<int> &ids)
{
  if (this->m_renderingStopped)
    return false;

  ids.clear();

  bool intersection(false);
  tgRay ray;
  GetCamera().GetViewRay(x, y, ray.start, ray.dir);

  dataMutex.Lock();

  // Get shortest distance to each model
  //  std::vector<double> dist(m_models3D.size(), DBL_MAX);
  for (unsigned i = 0; i < m_models3D.size(); i++)
  {
    if (m_models3D[i] == NULL)
      continue;
    std::vector<vec3> pl, nl;
    std::vector<double> zl;
    bool tmp_intersect = tgCollission::IntersectRayModel(pl, nl, zl, ray, *m_models3D[i]);
    intersection = (intersection || tmp_intersect);

    if (tmp_intersect)
      ids.push_back(i);
  }

  dataMutex.Unlock();

  return intersection;
}


bool tgTomGineThread::SelectModels(int x, int y, const std::vector<int> &model_ids, int& id, double& z_min)
{
  if (this->m_renderingStopped)
    return false;

  bool intersection(false);
  tgRay ray;
  GetCamera().GetViewRay(x, y, ray.start, ray.dir);

  dataMutex.Lock();

  // Get shortest distance to each model that intersects
  std::vector<double> dist(model_ids.size(), DBL_MAX);
  for (unsigned i = 0; i < model_ids.size(); i++)
  {
    const int& idx = model_ids[i];
    if (m_models3D[idx] == NULL)
      continue;
    std::vector<vec3> pl, nl;
    std::vector<double> zl;
    bool tmp_intersect = tgCollission::IntersectRayModel(pl, nl, zl, ray, *m_models3D[idx]);
    intersection = (intersection || tmp_intersect);
    if (tmp_intersect)
    {
      double z_min(DBL_MAX);
      for (unsigned j = 0; j < zl.size(); j++)
        if (zl[j] < z_min)
          z_min = zl[j];

      dist[i] = z_min;
    }
  }

  // get closest model id
  z_min = DBL_MAX;
  if (intersection)
  {
    for (unsigned i = 0; i < dist.size(); i++)
    {
      if (dist[i] < z_min)
      {
        z_min = dist[i];
        id = model_ids[i];
      }
    }
  }
  dataMutex.Unlock();

  return intersection;
}

bool tgTomGineThread::SelectModel(int x, int y, int &id)
{
  if (this->m_renderingStopped)
    return false;

  bool intersection(false);
  tgRay ray;
  GetCamera().GetViewRay(x, y, ray.start, ray.dir);

  dataMutex.Lock();

  // Get shortest distance to each model that intersects
  std::vector<double> dist(m_models3D.size(), DBL_MAX);
  for (unsigned i = 0; i < m_models3D.size(); i++)
  {
    if (m_models3D[i] == NULL)
      continue;
    std::vector<vec3> pl, nl;
    std::vector<double> zl;
    bool tmp_intersect = tgCollission::IntersectRayModel(pl, nl, zl, ray, *m_models3D[i]);
    intersection = (intersection || tmp_intersect);
    if (tmp_intersect)
    {
      double z_min(DBL_MAX);
      for (unsigned j = 0; j < zl.size(); j++)
        if (zl[j] < z_min)
          z_min = zl[j];

      dist[i] = z_min;
    }
  }

  // get closest model id
  if (intersection)
  {
    double z_min(DBL_MAX);
    for (unsigned i = 0; i < dist.size(); i++)
    {
      if (dist[i] < z_min)
      {
        z_min = dist[i];
        id = i;
      }
    }
  }
  dataMutex.Unlock();

  return intersection;
}

TomGine::tgCamera tgTomGineThread::GetCamera()
{
  //  Update();
  //  renderFinishedSem.Wait();
  TomGine::tgCamera cam;
  dataMutex.Lock();
  if(this->m_engine!=NULL)
    cam = this->m_engine->GetCamera();
  dataMutex.Unlock();
  return cam;
}

void tgTomGineThread::SetClearColor(float r, float g, float b)
{
  dataMutex.Lock();
  m_clearColor = TomGine::vec3(r, g, b);
  dataMutex.Unlock();
}

void tgTomGineThread::SetClearColor(float gray)
{
  dataMutex.Lock();
  m_clearColor = TomGine::vec3(gray, gray, gray);
  dataMutex.Unlock();
}

void tgTomGineThread::SetCamera(cv::Mat &_intrinsic)
{
  dataMutex.Lock();
  _intrinsic.copyTo(m_intrinsic);
  m_camChanged = true;
  dataMutex.Unlock();
}

void tgTomGineThread::SetCamera(cv::Mat &R, cv::Mat &t)
{
  dataMutex.Lock();
  R.copyTo(m_extR);
  t.copyTo(m_extT);
  m_camChanged = true;
  dataMutex.Unlock();
}

void tgTomGineThread::SetCamera(const TomGine::tgCamera &cam)
{
  dataMutex.Lock();
  //  m_tgCamera = cam;
  //  m_tgCamChanged = true;
  if(this->m_engine!=NULL)
    this->m_engine->SetCamera(cam);
  dataMutex.Unlock();
}

void tgTomGineThread::SetCameraDefault()
{
  dataMutex.Lock();
  m_extR = cv::Mat::eye(3, 3, CV_32F);
  m_extT = cv::Mat::zeros(3, 1, CV_32F);
  m_intrinsic = cv::Mat::zeros(3, 3, CV_64F);
  m_intrinsic.at<double> (0, 0) = m_intrinsic.at<double> (1, 1) = 525;
  m_intrinsic.at<double> (0, 2) = 320;
  m_intrinsic.at<double> (1, 2) = 240;
  m_intrinsic.at<double> (2, 2) = 1;
  m_camChanged = true;
  dataMutex.Unlock();
}

void tgTomGineThread::LookAt(const TomGine::vec3& p)
{
  TomGine::tgCamera cam = this->GetCamera();

  dataMutex.Lock();
  if(this->m_engine!=NULL)
  {
    // set extrinsic
    cam.LookAt(p);
    cam.ApplyTransform();

    // check if point is within z-range
    vec3 view_ray = cam.GetPos() - p;
    float z = view_ray.length();
    if(z >= cam.GetZFar())
      cam.SetZRange(cam.GetZNear(), 2.0f*z);
    if(z <= cam.GetZNear())
      cam.SetZRange(z*0.5f, cam.GetZFar());

    this->m_engine->SetCamera(cam);
    //  m_tgCamChanged = true;
  }
  dataMutex.Unlock();
}

void tgTomGineThread::SetRotationCenter(const cv::Vec3d &_rotCenter)
{
  dataMutex.Lock();
  m_rotCenter = vec3((float) _rotCenter[0], (float) _rotCenter[1], (float) _rotCenter[2]);
  m_rotCenterChanged = true;
  dataMutex.Unlock();
}
void tgTomGineThread::SetRotationCenter(float x, float y, float z)
{
  dataMutex.Lock();
  m_rotCenter = vec3(x, y, z);
  m_rotCenterChanged = true;
  dataMutex.Unlock();
}
void tgTomGineThread::SetRotationCenter(const TomGine::vec3 &cor)
{
  dataMutex.Lock();
  m_rotCenter = cor;
  m_rotCenterChanged = true;
  dataMutex.Unlock();
}

void tgTomGineThread::SetCoordinateFrame()
{
  dataMutex.Lock();
  m_showCoordinateFrame = !m_showCoordinateFrame;
  dataMutex.Unlock();
}

void tgTomGineThread::SetImage(unsigned char* data, unsigned width, unsigned height)
{
  cv::Mat _img(height, width, CV_8UC3, data);

  dataMutex.Lock();
  _img.copyTo(m_img);
  m_loadImage = true;
  dataMutex.Unlock();
}

void tgTomGineThread::SetImage(const cv::Mat &_img, bool show, bool background)
{
  if (_img.channels() != 3 && _img.channels() != 1)
  {
    printf("[tgTomGineThread::SetImage] Warning, image is no rgb image\n");
    return;
  }

  dataMutex.Lock();
  _img.copyTo(m_img);
  m_loadImage = true;
  df.image = show;
  dataMutex.Unlock();
}

bool tgTomGineThread::GetImage(cv::Mat &_img)
{
  if (m_img.empty())
    return false;
  else
  {
    m_img.copyTo(_img);
    return true;
  }
}

void tgTomGineThread::SetInputSpeeds(float rotation, float translation, float zoom)
{
  dataMutex.Lock();
  m_inputSpeeds = vec3(rotation, translation, zoom);
  m_inputSpeedChanged = true;
  dataMutex.Unlock();
}

void tgTomGineThread::GetInputSpeeds(float &rotation, float &translation, float &zoom)
{
  dataMutex.Lock();
  rotation = m_inputSpeeds.x;
  translation = m_inputSpeeds.y;
  zoom = m_inputSpeeds.z;
  dataMutex.Unlock();
}

void tgTomGineThread::SetNormalLength(float n)
{
  dataMutex.Lock();
  m_normal_length = n;
  dataMutex.Unlock();
}

void tgTomGineThread::SetFrameRate(unsigned max_fps)
{
  dataMutex.Lock();
  m_maxFPS = max_fps;
  m_FPS = !(m_maxFPS == 0);
  dataMutex.Unlock();
}

int tgTomGineThread::AddCamera(const tgCamera& cam)
{
  dataMutex.Lock();
  int id = m_cameras.size();
  m_cameras.push_back(cam);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPoint2D(float x, float y, uchar r, uchar g, uchar b, float size)
{
  if (size <= 0.0f)
  {
    printf("tgTomGineThread::AddPoint2D] Warning invalid argument: size <= 0.0f\n");
    size = 1.0f;
  }
  dataMutex.Lock();
  tgColorPoint pt;
  pt.pos = vec3(x, y, 0.0f);
  pt.color[0] = r;
  pt.color[1] = g;
  pt.color[2] = b;
  m_points2D.push_back(pt);
  m_pointSize2D.push_back(size);
  int id = (this->m_points2D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPoint3D(float x, float y, float z, uchar r, uchar g, uchar b, float size)
{
  if (size <= 0.0f)
  {
    printf("tgTomGineThread::AddPoint3D] Warning invalid argument: size <= 0.0f\n");
    size = 1.0f;
  }
  dataMutex.Lock();
  tgColorPoint pt;
  pt.pos = vec3(x, y, z);
  pt.color[0] = r;
  pt.color[1] = g;
  pt.color[2] = b;
  m_points3D.push_back(pt);
  m_pointSize3D.push_back(size);
  int id = (this->m_points3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLine2D(float x1, float y1, float x2, float y2, uchar r, uchar g, uchar b, float width)
{
  if (width <= 0.0f)
  {
    printf("tgTomGineThread::AddLine2D] Warning invalid argument: width <= 0.0f\n");
    width = 1.0f;
  }
  dataMutex.Lock();
  m_lines2D.push_back(TomGine::tgLine(vec3(x1, y1, 0.0), vec3(x2, y2, 0.0)));
  m_lineCols2D.push_back(vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f));
  m_lineWidth2D.push_back(width);
  int id = (this->m_lines2D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLine3D(float x1, float y1, float z1, float x2, float y2, float z2,
                               uchar r, uchar g, uchar b, float width)
{
  if (width <= 0.0f)
  {
    printf("tgTomGineThread::AddLine3D] Warning invalid argument: width <= 0.0f\n");
    width = 1.0f;
  }
  dataMutex.Lock();
  m_lines3D.push_back(TomGine::tgLine(vec3(x1, y1, z1), vec3(x2, y2, z2)));
  m_lineCols3D.push_back(vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f));
  m_lineWidth3D.push_back(width);
  int id = (this->m_lines3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLine3D(const TomGine::tgLine& line,
                               uchar r, uchar g, uchar b, float width)
{
  if (width <= 0.0f)
  {
    printf("tgTomGineThread::AddLine3D] Warning invalid argument: width <= 0.0f\n");
    width = 1.0f;
  }
  dataMutex.Lock();
  m_lines3D.push_back(line);
  m_lineCols3D.push_back(vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f));
  m_lineWidth3D.push_back(width);
  int id = (this->m_lines3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLabel2D(std::string text, int size, int x, int y, float r, float g, float b)
{
  dataMutex.Lock();
  m_labels2D.push_back(TomGine::tgLabel2D(text, size, x, y, r, g, b));
  int id = (this->m_labels2D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLabel3D(std::string text, int size, vec3 pos, float r, float g, float b)
{
  dataMutex.Lock();
  TomGine::tgLabel3D label;
  label.text = text;
  label.size = size;
  label.pos = pos;
  label.rgba = vec4(r, g, b, 1.0f);
  m_labels3D.push_back(label);
  int id = (this->m_labels3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddLabel3D(std::string text, int size, double x, double y, double z)
{
  dataMutex.Lock();
  TomGine::tgLabel3D label;
  label.text = text;
  label.size = size;
  label.pos.x = x;
  label.pos.y = y;
  label.pos.z = z;
  m_labels3D.push_back(label);
  int id = (this->m_labels3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddModel2D(const TomGine::tgTextureModel &model)
{
  dataMutex.Lock();
  this->m_models2D.push_back(new TomGine::tgTextureModel(model));
  int id = (this->m_models2D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddModel3D(const TomGine::tgTextureModel &model)
{
  dataMutex.Lock();
  int id = this->m_models3D.size();
  this->m_models3D.push_back(new TomGine::tgTextureModel(model));
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddModel(const TomGine::tgTextureModel &model)
{
  dataMutex.Lock();
  this->m_models3D.push_back(new TomGine::tgTextureModel(model));
  int id = (this->m_models3D.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddModel(TomGine::tgModel *model)
{
  dataMutex.Lock();
  this->m_modelpointers.push_back(model);
  int id = (this->m_modelpointers.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPointCloud(const std::vector<TomGine::vec3> &cloud, TomGine::vec3 color, float pointsize)
{
  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_point_size = pointsize;
  tg_cloud->m_colorpoints.resize(cloud.size());

  for (size_t i = 0; i < cloud.size(); i++)
  {
      TomGine::tgColorPoint& cpt = tg_cloud->m_colorpoints[i];
      const TomGine::vec3 &pt = cloud[i];
      cpt.color[0] = static_cast<unsigned char>(255*color.x);
      cpt.color[1] = static_cast<unsigned char>(255*color.y);
      cpt.color[2] = static_cast<unsigned char>(255*color.z);
      cpt.pos = vec3(pt[0], pt[1], pt[2]);
  }
  dataMutex.Lock();
  this->m_pointclouds.push_back(tg_cloud);
  int id = (this->m_pointclouds.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPointCloud(const cv::Mat_<cv::Vec4f>& cloud, float pointsize)
{
  tgRGBValue color;
  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_point_size = pointsize;
  tg_cloud->m_colorpoints.resize(cloud.rows*cloud.cols);

  for (int i = 0; i < cloud.rows; i++)
  {
    for (int j = 0; j < cloud.cols; j++)
    {
      TomGine::tgColorPoint& cpt = tg_cloud->m_colorpoints[i*cloud.cols + j];
      const cv::Vec4f &pt = cloud(i, j);
      color.float_value = pt[3];
      cpt.color[0] = color.Red;
      cpt.color[1] = color.Green;
      cpt.color[2] = color.Blue;
      cpt.pos = vec3(pt[0], pt[1], pt[2]);
    }
  }
  dataMutex.Lock();
  this->m_pointclouds.push_back(tg_cloud);
  int id = (this->m_pointclouds.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPointCloud(const std::vector<cv::Vec4f> &cloud, float pointsize)
{
  tgRGBValue color;
  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_point_size = pointsize;
  tg_cloud->m_colorpoints.resize(cloud.size());

  for (unsigned i = 0; i < cloud.size(); i++)
  {
    TomGine::tgColorPoint& cpt = tg_cloud->m_colorpoints[i];
    const cv::Vec4f &pt = cloud[i];
    color.float_value = pt[3];
    cpt.color[0] = color.Red;
    cpt.color[1] = color.Green;
    cpt.color[2] = color.Blue;
    cpt.pos = vec3(pt[0], pt[1], pt[2]);
  }
  dataMutex.Lock();
  this->m_pointclouds.push_back(tg_cloud);
  int id = (this->m_pointclouds.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPointCloud(const cv::Mat_<cv::Vec3f> &cloud, uchar r, uchar g, uchar b, float pointsize)
{ 
  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_point_size = pointsize;
  tg_cloud->m_colorpoints.resize(cloud.rows*cloud.cols);

  for (int i = 0; i < cloud.rows*cloud.cols; i++)
  {
    TomGine::tgColorPoint& cpt = tg_cloud->m_colorpoints[i];
    const cv::Vec3f &pt = cloud(i);
    cpt.color[0] = r;
    cpt.color[1] = g;
    cpt.color[2] = b;
    cpt.pos = vec3(pt[0], pt[1], pt[2]);
  }
  dataMutex.Lock();
  this->m_pointclouds.push_back(tg_cloud);
  int id = (this->m_pointclouds.size() - 1);
  dataMutex.Unlock();
  return id;
}

int tgTomGineThread::AddPointCloud(const cv::Mat_<cv::Vec3f> &cloud, const cv::Mat_<cv::Vec3b> &image, float pointsize)
{ 
  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_point_size = pointsize;
  int s = std::min<int>(cloud.rows*cloud.cols, image.rows*image.cols);
  tg_cloud->m_colorpoints.resize(s);
  
  for (int i = 0; i < s; i++)
  {
    TomGine::tgColorPoint& cpt = tg_cloud->m_colorpoints[i];
    const cv::Vec3f &pt = cloud(i);
    const cv::Vec3b &col = image(i);
    cpt.color[0] = col[2];
    cpt.color[1] = col[1];
    cpt.color[2] = col[0];
    cpt.pos = vec3(pt[0], pt[1], pt[2]);
  }
  dataMutex.Lock();
  this->m_pointclouds.push_back(tg_cloud);
  int id = (this->m_pointclouds.size() - 1);
  dataMutex.Unlock();
  return id;
}

void tgTomGineThread::SetPoint2D(int id, float x, float y)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_points2D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetPoint2D] Warning index out of bounds: %d.\n", id);
    return;
  }
  TomGine::tgColorPoint &pt = this->m_points2D[id];
  pt.pos.x = x;
  pt.pos.y = y;
  dataMutex.Unlock();
}
void tgTomGineThread::SetPoint3D(int id, float x, float y, float z)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_points3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetPoint3D] Warning index out of bounds: %d.\n", id);
    return;
  }
  m_points3D[id].pos.x = x;
  m_points3D[id].pos.y = y;
  m_points3D[id].pos.z = z;
  dataMutex.Unlock();
}

void tgTomGineThread::SetLine2D(int id,
                                float x1, float y1, float x2, float y2,
                                uchar r, uchar g, uchar b, float width)
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_lines2D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetLine2D] Warning index out of bounds: %d.\n", id);
    return;
  }
  m_lines2D[id] = TomGine::tgLine(vec3(x1, y1, 0.0), vec3(x2, y2, 0.0));
  m_lineCols2D[id] = vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
  m_lineWidth2D[id] = width;
  dataMutex.Unlock();
}

void tgTomGineThread::SetLine3D(int id,
                                float x1, float y1, float z1, float x2, float y2, float z2,
                                uchar r, uchar g, uchar b, float width)
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_lines3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetLine3D] Warning index out of bounds: %d.\n", id);
    return;
  }
  m_lines3D[id] = TomGine::tgLine(vec3(x1, y1, z1), vec3(x2, y2, z2));
  m_lineCols3D[id] = vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
  m_lineWidth3D[id] = width;
  dataMutex.Unlock();
}

void tgTomGineThread::SetLine3D(int id, const TomGine::tgLine& line,
                                uchar r, uchar g, uchar b, float width)
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_lines3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetLine3D] Warning index out of bounds: %d.\n", id);
    return;
  }
  m_lines3D[id] = line;
  m_lineCols3D[id] = vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
  m_lineWidth3D[id] = width;
  dataMutex.Unlock();
}

void tgTomGineThread::SetLabel2D(int id, std::string text, int size, int x, int y, float r, float g, float b)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_labels2D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetLabel2D] Warning index out of bounds: %d.\n", id);
    return;
  }
  this->m_labels2D[id] = TomGine::tgLabel2D(text, size, x, y, r, g, b);
  dataMutex.Unlock();
}

void tgTomGineThread::SetModel2D(int id, const TomGine::tgTextureModel &model)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_models2D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetModel2D] Warning index out of bounds: %d.\n", id);
    return;
  }
  if (m_models2D[id] != NULL)
    delete m_models2D[id];
  m_models2D[id] = new TomGine::tgTextureModel(model);
  dataMutex.Unlock();
}

void tgTomGineThread::SetModel3D(int id, const TomGine::tgTextureModel &model)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_models3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetModel] Warning index out of bounds: %d.\n", id);
    return;
  }
  if (m_models3D[id] != NULL)
    delete m_models3D[id];
  m_models3D[id] = new TomGine::tgTextureModel(model);
  dataMutex.Unlock();
}

void tgTomGineThread::SetModelPose(int id, const TomGine::tgPose &pose)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_models3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetModelPose] Warning index out of bounds: %d.\n", id);
    return;
  }
  this->m_models3D[id]->m_pose = pose;
  dataMutex.Unlock();
}


void tgTomGineThread::SetModelColor(int id, uchar r, uchar g, uchar b)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_models3D.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetModelColor] Warning index out of bounds: %d.\n", id);
    return;
  }
  this->m_models3D[id]->SetColor(r,g,b);
  dataMutex.Unlock();
}

void tgTomGineThread::SetModel(int id, TomGine::tgModel *model)
{
  if (this->m_renderingStopped)
    return;
  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_modelpointers.size())
  {
    dataMutex.Unlock();
    printf("[tgTomGineThread::SetModel] Warning index out of bounds: %d.\n", id);
    return;
  }
  this->m_modelpointers[id] = model;
  dataMutex.Unlock();
}

void tgTomGineThread::SetPointCloud(int id, const TomGine::tgModel &pcl)
{
  if (this->m_renderingStopped)
    return;

  tgModel* tg_cloud = new tgModel;
  tg_cloud->m_colorpoints = pcl.m_colorpoints;

  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_pointclouds.size())
  {
    dataMutex.Unlock();
    if (this->m_renderingStopped)
      return;
    printf("[tgTomGineThread::SetPointCloud] Warning index out of bounds: %d.\n", id);
    return;
  }
  if (m_pointclouds[id] != NULL)
    delete m_pointclouds[id];
  m_pointclouds[id] = tg_cloud;
  dataMutex.Unlock();
}

void tgTomGineThread::SetPointCloud(int id, cv::Mat_<cv::Vec4f> cloud)
{
  if (this->m_renderingStopped)
    return;

  tgRGBValue color;
  tgModel* tg_cloud = new tgModel;

  for (int i = 0; i < cloud.rows; i++)
  {
    for (int j = 0; j < cloud.cols; j++)
    {
      TomGine::tgColorPoint cpt;
      cv::Vec4f &pt = cloud(i, j);
      color.float_value = pt[3];
      cpt.color[0] = color.Red;
      cpt.color[1] = color.Green;
      cpt.color[2] = color.Blue;
      cpt.pos = vec3(pt[0], pt[1], pt[2]);
      tg_cloud->m_colorpoints.push_back(cpt);
    }
  }

  dataMutex.Lock();
  if (id < 0 || id >= (int) this->m_pointclouds.size())
  {
    if (this->m_renderingStopped)
    {
      dataMutex.Unlock();
      return;
    } else
    {
      dataMutex.Unlock();
      printf("[tgTomGineThread::SetPointCloud] Warning index out of bounds: %d.\n", id);
      return;
    }
  }
  if (m_pointclouds[id] != NULL)
    delete m_pointclouds[id];
  m_pointclouds[id] = tg_cloud;
  dataMutex.Unlock();
}

void tgTomGineThread::Update()
{
    if (this->m_renderingStopped)
        return;

  //Event event;
  //event.type = TMGL_None;
  //eventMutex.Lock();
  //m_eventlist.push_back(event);
  //Update();
  //eventMutex.Unlock();

    bool engineNULL = true;
    while (engineNULL)
    {
        dataMutex.Lock();
        engineNULL = (m_engine == NULL);
        dataMutex.Unlock();

        Sleep(100);
    }

    this->m_engine->UnWaitForEvent();

    renderFinishedSem.Wait();
}

bool tgTomGineThread::Stopped()
{
  dataMutex.Lock();
  bool stopped = (this->m_renderingStopped);
  dataMutex.Unlock();
  return stopped;
}

void tgTomGineThread::Snapshot(const char* filename)
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  m_snapshotFile = std::string(filename);
  m_snapshot = true;
  Update();
  dataMutex.Unlock();

  snapshotSem.Wait();
}

void tgTomGineThread::StartRecording(std::string path, unsigned fps)
{
  if (this->m_renderingStopped)
    return;

  SetFrameRate(fps);
  dataMutex.Lock();
  m_recorderFrame = 0;
  m_recorderPath = path;
  m_record = true;
  Update();
  dataMutex.Unlock();
}

void tgTomGineThread::StopRecording()
{
  if(this->m_renderingStopped)
    return;

  SetFrameRate(0);
  dataMutex.Lock();
  m_record = false;
  dataMutex.Unlock();
}

void tgTomGineThread::GetDepthBuffer(cv::Mat1f &z, TomGine::tgCamera &cam)
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  this->m_waitForZBuffer = true;
  Update();
  dataMutex.Unlock();

  renderFinishedSem.Wait();

  dataMutex.Lock();
  this->m_zBuffer.copyTo(z);
  cam = this->m_engine->GetCamera();
  dataMutex.Unlock();
}

void tgTomGineThread::Clear()
{
  if (this->m_renderingStopped)
    return;

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_ALL;
  m_clearindex = 0;
  dataMutex.Unlock();
  
  Update();

  clearFinishedSem.Wait();

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_NONE;
  dataMutex.Unlock();
}

void tgTomGineThread::ClearPointClouds()
{
  if(m_renderingStopped)
    return;

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_POINTCLOUDS;
  dataMutex.Unlock();

  Update();

  clearFinishedSem.Wait();

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_NONE;
  dataMutex.Unlock();
}

void tgTomGineThread::ClearCameras()
{
  dataMutex.Lock();
  m_cameras.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearPoints2D()
{
  dataMutex.Lock();
  m_points2D.clear();
  m_pointSize2D.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearPoints3D()
{
  dataMutex.Lock();
  m_points3D.clear();
  m_pointSize3D.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearLabels()
{
  dataMutex.Lock();
  m_labels2D.clear();
  m_labels3D.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearLines2D()
{
  dataMutex.Lock();
  m_lines2D.clear();
  m_lineCols2D.clear();
  m_lineWidth2D.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearLines3D()
{
  dataMutex.Lock();
  m_lines3D.clear();
  m_lineCols3D.clear();
  m_lineWidth3D.clear();
  dataMutex.Unlock();
}

void tgTomGineThread::ClearModels()
{
  if(m_renderingStopped)
    return;

  ClearModels2D();
  ClearModels3D();
  ClearModelPointers();
}

void tgTomGineThread::ClearModels2D()
{
  if(m_renderingStopped)
    return;

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_MODELS_2D;
  dataMutex.Unlock();

  Update();

  clearFinishedSem.Wait();

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_NONE;
  dataMutex.Unlock();
}

void tgTomGineThread::ClearModels3D()
{
  if(m_renderingStopped)
    return;

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_MODELS_3D;
  dataMutex.Unlock();

  Update();
  clearFinishedSem.Wait();

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_NONE;
  dataMutex.Unlock();
}

void tgTomGineThread::ClearModels3DFrom(int id)
{
  if(m_renderingStopped)
    return;

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_MODELS_3D;
  m_clearindex = id;
  dataMutex.Unlock();

  Update();
  clearFinishedSem.Wait();

  dataMutex.Lock();
  m_clearcommand = GL_CLEAR_NONE;
  m_clearindex = 0;
  dataMutex.Unlock();
}

void tgTomGineThread::ClearModelPointers()
{
  m_modelpointers.clear(); // vector of pointers cleared; the models are not deleted
}

void tgTomGineThread::DragDropModels(float speed)
{
  if(m_renderingStopped)
    return;

  this->StartEventListener(100);
  bool stopDragDrop(false);
  while (!this->Stopped() && !stopDragDrop)
  {
    int id;
    std::list<TomGine::Event> events;
    this->GetEventQueue(events);
    while (!events.empty())
    {

      TomGine::Event ev = events.front();
      events.pop_front();

      if (ev.type == TomGine::TMGL_Press && ev.input == TomGine::TMGL_Return)
      {
        return;
      }

      if (ev.type == TomGine::TMGL_Release && ev.input == TomGine::TMGL_x)
      {
        bool return1(false);
        bool return2(false);

        TomGine::Event::Motion motion;
        motion.x = ev.motion.x;
        motion.y = ev.motion.y;
        this->SelectModel(ev.motion.x, ev.motion.y, id);

        // look if a release event is in event que
        while (!events.empty())
        {
          ev = events.front();
          events.pop_front();
          if (ev.type == TomGine::TMGL_Release && ev.input == TomGine::TMGL_x)
            return1 = true;
        }

        if (!return1)
        {
          while (!return2)
          {

            // move object
            this->GetEventQueue(events);
            while (!events.empty())
            {
              ev = events.front();
              events.pop_front();

              if (ev.type == TomGine::TMGL_Release && ev.input == TomGine::TMGL_x)
                return2 = true;

              else if (ev.type == TomGine::TMGL_Motion)
              {

                motion.x = ev.motion.x - motion.x;
                motion.y = ev.motion.y - motion.y;

                dataMutex.Lock();
                m_models3D[id]->m_pose.Translate(TomGine::vec3(speed * motion.x, speed * motion.y, 0.0));
                dataMutex.Unlock();

                motion.x = ev.motion.x;
                motion.y = ev.motion.y;

              }
            }

          }

        }
      }

    }
  }
  this->StopEventListener();
}

