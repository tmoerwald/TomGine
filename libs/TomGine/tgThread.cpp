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
 
#include "tgThread.h"

using namespace TomGine;
#ifdef WIN32

tgThread::tgThread(void (*start_routine)(void*), void *arg)
    : m_thread(start_routine, arg)
{    
}

TomGine::tgThread::~tgThread()
{
}

void tgThread::Join()
{
    m_thread.join();
}

tgMutex::tgMutex()
{
}

tgMutex::~tgMutex()
{
}

void tgMutex::Lock()
{
    m_mutex.lock();
}

void tgMutex::Unlock()
{
    m_mutex.unlock();
}

tgSemaphore::tgSemaphore()
    : m_sem(0)
{

}

tgSemaphore::~tgSemaphore()
{

}

void tgSemaphore::Post()
{
    m_sem.post();
}

void tgSemaphore::Wait()
{
    m_sem.wait();
}

void tgSemaphore::TryWait()
{
    m_sem.try_wait();
}
#endif

#ifdef LINUX
tgThread::tgThread(void *(*start_routine)(void*), void *arg)
{
    pthread_create(&m_thread, NULL, start_routine, arg);
}

TomGine::tgThread::~tgThread()
{
    pthread_mutex_destroy(&m_thread);
}

void tgThread::Join()
{
    pthread_join(m_thread, NULL);
}

tgMutex::tgMutex()
{
    pthread_mutex_init(&m_mutex, NULL);
}

tgMutex::~tgMutex()
{
}

void tgMutex::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

void tgMutex::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

tgSemaphore::tgSemaphore()
{
    sem_init(&m_sem, 0, 0);
}

tgSemaphore::~tgSemaphore()
{
    sem_destroy(&m_sem);
}

void tgSemaphore::Post()
{
    sem_post(&m_sem);
}

void tgSemaphore::Wait()
{
    sem_wait(&m_sem);
}

void tgSemaphore::TryWait()
{
    sem_trywait(&m_sem);
}

#endif