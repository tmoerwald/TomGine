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
 
#ifndef TG_THREAD
#define TG_THREAD

#include "headers.h"

#ifdef LINUX
#include <semaphore.h>
#endif

#ifdef WIN32
#include <boost/thread/thread.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#endif

namespace TomGine
{
    class tgThread
    {
    public:
        tgThread(void (*start_routine)(void*), void *arg);
        ~tgThread();

        void Join();

    private:
#ifdef LINUX
        pthread_t m_thread;
#endif

#ifdef WIN32
        boost::thread m_thread;
#endif
    };

    class tgMutex
    {
    public:
        tgMutex();
        ~tgMutex();

        void Lock();
        void Unlock();

    private:
#ifdef LINUX
        pthread_mutex_t m_mutex;
#endif

#ifdef WIN32
        boost::mutex m_mutex;
#endif
    };

    class tgSemaphore
    {
    public:
        tgSemaphore();
        ~tgSemaphore();

        void Post();
        void Wait();
        void TryWait();

    private:
#ifdef LINUX
        sem_t m_sem;
#endif

#ifdef WIN32
        boost::interprocess::interprocess_semaphore m_sem;
#endif
    };

} // namespace TomGine

#endif
