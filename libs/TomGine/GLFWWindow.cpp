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

#ifdef WIN32

#include "GLWindow.h"
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <queue>

using namespace TomGine;

std::queue<Event> eventqueue;

bool MapType(int key, Type& type);
bool MapKeyInput(int key, Input& input);
bool MapMouseInput(int button, Input& input);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

GLWindow::GLWindow(unsigned int width, unsigned int height, const char* name, bool threaded, bool stereo)
{
    if (!glfwInit())
        exit(EXIT_FAILURE);

    window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        std::cout << "[GLWindow::GLWindow] Error creating GLFW OpenGL context.\n";
        exit(EXIT_FAILURE);
    }

    glfwSetWindowAspectRatio(window, width, height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // create font
    hWnd = glfwGetWin32Window(window);
    hDC = GetDC(hWnd);
    font_base = 1000;
    if (!wglUseFontBitmaps(hDC, 0, 256, font_base))
    {
        DWORD dw = GetLastError();
        std::cout << "Error using font bitmaps: " << dw << std::endl;
        throw std::runtime_error("Error using font bitmaps.");
    }
}

GLWindow::~GLWindow()
{
    glfwMakeContextCurrent(window);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLWindow::Activate()
{
    glfwMakeContextCurrent(window);
}

void GLWindow::Update()
{
    glfwSwapBuffers(window);
}

bool GLWindow::GetEvent(Event &event)
{
    glfwPollEvents();

    if (eventqueue.empty())
    {
        return false;
    }

    event = eventqueue.front();
    eventqueue.pop();
    return true;
}

void GLWindow::GetEventBlocking(Event &event)
{
    if (!eventqueue.empty())
    {
        event = eventqueue.front();
        eventqueue.pop();
        return;
    }

    glfwWaitEvents();

    GLFWwindow* window = glfwGetCurrentContext();
    if (glfwWindowShouldClose(window))
    {
        event.type = TMGL_Quit;
        return;
    }

    if (eventqueue.empty())
    {
        event.type = TMGL_None;
    }
    else
    {
        event = eventqueue.front();
        eventqueue.pop();
    }
}

void GLWindow::UnBlockGetEvent()
{
    glfwPostEmptyEvent();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Event event;
    event.type = TMGL_Expose;
    event.expose.width = width;
    event.expose.height = height;
    eventqueue.push(event);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Event event;
    if (MapType(action, event.type) && MapKeyInput(key, event.input))
    {
        eventqueue.push(event);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Event event;
    if (MapType(action, event.type) && MapMouseInput(button, event.input))
    {
        eventqueue.push(event);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    Event event;
    event.type = TMGL_Motion;
    event.motion.x = (int)xpos;
    event.motion.y = (int)ypos;
    eventqueue.push(event);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Event event;
    event.type = TMGL_Press;
    if (yoffset < 0)
        event.input = TMGL_Button4;
    else if (yoffset > 0.0)
        event.input = TMGL_Button5;
    else
        return;

    eventqueue.push(event);
}

bool MapType(int action, Type& type)
{
    switch (action)
    {
    case GLFW_PRESS:
        type = TMGL_Press;
        return true;
    case GLFW_RELEASE:
        type = TMGL_Release;
        return true;
    default:
        return false;
    }
    return false;
}

bool MapKeyInput(int key, Input& input)
{
    switch (key)
    {
    case GLFW_KEY_0:
        input = TMGL_0;
        break;
    case GLFW_KEY_1:
        input = TMGL_1;
        break;
    case GLFW_KEY_2:
        input = TMGL_2;
        break;
    case GLFW_KEY_3:
        input = TMGL_3;
        break;
    case GLFW_KEY_4:
        input = TMGL_4;
        break;
    case GLFW_KEY_5:
        input = TMGL_5;
        break;
    case GLFW_KEY_6:
        input = TMGL_6;
        break;
    case GLFW_KEY_7:
        input = TMGL_7;
        break;
    case GLFW_KEY_8:
        input = TMGL_8;
        break;
    case GLFW_KEY_9:
        input = TMGL_9;
        break;

    case GLFW_KEY_A:
        input = TMGL_a;
        break;
    case GLFW_KEY_B:
        input = TMGL_b;
        break;
    case GLFW_KEY_C:
        input = TMGL_c;
        break;
    case GLFW_KEY_D:
        input = TMGL_d;
        break;
    case GLFW_KEY_E:
        input = TMGL_e;
        break;
    case GLFW_KEY_F:
        input = TMGL_f;
        break;
    case GLFW_KEY_G:
        input = TMGL_g;
        break;
    case GLFW_KEY_H:
        input = TMGL_h;
        break;
    case GLFW_KEY_I:
        input = TMGL_i;
        break;
    case GLFW_KEY_J:
        input = TMGL_j;
        break;
    case GLFW_KEY_K:
        input = TMGL_k;
        break;
    case GLFW_KEY_L:
        input = TMGL_l;
        break;
    case GLFW_KEY_M:
        input = TMGL_m;
        break;
    case GLFW_KEY_N:
        input = TMGL_n;
        break;
    case GLFW_KEY_O:
        input = TMGL_o;
        break;
    case GLFW_KEY_P:
        input = TMGL_p;
        break;
    case GLFW_KEY_Q:
        input = TMGL_q;
        break;
    case GLFW_KEY_R:
        input = TMGL_r;
        break;
    case GLFW_KEY_S:
        input = TMGL_s;
        break;
    case GLFW_KEY_T:
        input = TMGL_t;
        break;
    case GLFW_KEY_U:
        input = TMGL_u;
        break;
    case GLFW_KEY_V:
        input = TMGL_v;
        break;
    case GLFW_KEY_W:
        input = TMGL_w;
        break;
    case GLFW_KEY_X:
        input = TMGL_x;
        break;
    case GLFW_KEY_Y:
        input = TMGL_y;
        break;
    case GLFW_KEY_Z:
        input = TMGL_z;
        break;

    case GLFW_KEY_BACKSPACE:
        input = TMGL_BackSpace;
        break;
    case GLFW_KEY_TAB:
        input = TMGL_Tab;
        break;
    case GLFW_KEY_ENTER:
        input = TMGL_Return;
        break;
    case GLFW_KEY_SPACE:
        input = TMGL_Space;
        break;
    case GLFW_KEY_PAUSE:
        input = TMGL_Pause;
        break;
    case GLFW_KEY_ESCAPE:
        input = TMGL_Escape;
        break;
    case GLFW_KEY_DELETE:
        input = TMGL_Delete;
        break;
    case GLFW_KEY_LEFT:
        input = TMGL_Left;
        break;
    case GLFW_KEY_UP:
        input = TMGL_Up;
        break;
    case GLFW_KEY_RIGHT:
        input = TMGL_Right;
        break;
    case GLFW_KEY_DOWN:
        input = TMGL_Down;
        break;
    case GLFW_KEY_PAGE_UP:
        input = TMGL_Page_Up;
        break;
    case GLFW_KEY_PAGE_DOWN:
        input = TMGL_Page_Down;
        break;
    case GLFW_KEY_END:
        input = TMGL_End;
        break;
    case GLFW_KEY_HOME:
        input = TMGL_Home;
        break;
    case GLFW_KEY_KP_MULTIPLY:
        input = TMGL_KP_Multiply;
        break;
    case GLFW_KEY_KP_ADD:
        input = TMGL_KP_Add;
        break;
    case GLFW_KEY_KP_SUBTRACT:
        input = TMGL_KP_Subtract;
        break;
    case GLFW_KEY_KP_DIVIDE:
        input = TMGL_KP_Divide;
        break;
    //case :
    //    input = TMGL_KP_0;
    //    break;
    //case VK_NUMPAD1:
    //    input = TMGL_KP_1;
    //    break;
    //case VK_NUMPAD2:
    //    input = TMGL_KP_2;
    //    break;
    //case VK_NUMPAD3:
    //    input = TMGL_KP_3;
    //    break;
    //case VK_NUMPAD4:
    //    input = TMGL_KP_4;
    //    break;
    //case VK_NUMPAD5:
    //    input = TMGL_KP_5;
    //    break;
    //case VK_NUMPAD6:
    //    input = TMGL_KP_6;
    //    break;
    //case VK_NUMPAD7:
    //    input = TMGL_KP_7;
    //    break;
    //case VK_NUMPAD8:
    //    input = TMGL_KP_8;
    //    break;
    //case VK_NUMPAD9:
    //    input = TMGL_KP_9;
    //    break;
    case GLFW_KEY_F1:
        input = TMGL_F1;
        break;
    case GLFW_KEY_F2:
        input = TMGL_F2;
        break;
    case GLFW_KEY_F3:
        input = TMGL_F3;
        break;
    case GLFW_KEY_F4:
        input = TMGL_F4;
        break;
    case GLFW_KEY_F5:
        input = TMGL_F5;
        break;
    case GLFW_KEY_F6:
        input = TMGL_F6;
        break;
    case GLFW_KEY_F7:
        input = TMGL_F7;
        break;
    case GLFW_KEY_F8:
        input = TMGL_F8;
        break;
    case GLFW_KEY_F9:
        input = TMGL_F9;
        break;
    case GLFW_KEY_F10:
        input = TMGL_F10;
        break;
    case GLFW_KEY_F11:
        input = TMGL_F11;
        break;
    case GLFW_KEY_F12:
        input = TMGL_F12;
        break;
    case GLFW_KEY_LEFT_SHIFT:
        input = TMGL_Shift_L;
        break;
    case GLFW_KEY_RIGHT_SHIFT:
        input = TMGL_Shift_R;
        break;
    case GLFW_KEY_LEFT_CONTROL:
        input = TMGL_Control_L;
        break;
    case GLFW_KEY_RIGHT_CONTROL:
        input = TMGL_Control_R;
        break;
    default:
        return false;
    }
    return true;
}

bool MapMouseInput(int button, Input& input)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        input = TMGL_Button1;
        return true;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        input = TMGL_Button2;
        return true;
    case GLFW_MOUSE_BUTTON_RIGHT:
        input = TMGL_Button3;
        return true;
    default:
        return false;

    }
    return false;
}

#endif /* WIN32 */
