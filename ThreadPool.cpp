#include "ThreadPool.h"

ThreadPool::ThreadPool() : m_pool_size(DEFAULT_POOL_SIZE)
{
}

ThreadPool::ThreadPool(int pool_size) : m_pool_size(pool_size)
{
}

ThreadPool::~ThreadPool()
{
  // Release resources
  if (m_pool_state != STOPPED) {
    destroy_threadpool();
  }
}

// wrapper function that calls the member function
// we want to run in the thread.
extern "C"
void* start_thread(void* arg)
{
    ThreadPool* tp = (ThreadPool*) arg;
    tp->execute_thread();
    
    return NULL;
}

int ThreadPool::initialize_threadpool()
{
    // TODO: Implement lazy loading threads instead of creating all at once
    m_pool_state = STARTED;
    for (int i = 0; i < m_pool_size; i++) 
    {
        pthread_t tid;
        if (pthread_create(&tid, NULL, start_thread, (void*) this) != 0) 
        {
            cerr << "pthread_create() failed" << endl;
            return -1;
        }
        m_threads.push_back(tid);
    }

    return 0;
}

int ThreadPool::destroy_threadpool()
{
  m_task_mutex.lock();
  m_pool_state = STOPPED;
  m_task_mutex.unlock();
  
  m_task_cond_var.broadcast(); // notify all threads to shut down

  for (int i = 0; i < m_pool_size; i++) {
    void* result;
    pthread_join(m_threads[i], &result);
    m_task_cond_var.broadcast(); // try waking up a bunch of threads that are still waiting
  }
  return 0;
}

void* ThreadPool::execute_thread()
{
  Task* task = NULL;

  while(true) 
  {
    m_task_mutex.lock();

    while ((m_pool_state != STOPPED) && (m_tasks.empty())) 
    {
      // Wait until there is a task in the queue
        m_task_cond_var.wait(m_task_mutex.get_mutex_ptr());
    }

    // notify process shutdown, return from here
    if (m_pool_state == STOPPED) {
      m_task_mutex.unlock();
      pthread_exit(NULL);
    }

    // get task
    task = m_tasks.front();
    m_tasks.pop_front();
    m_task_mutex.unlock();

    // execute the task
    task->run();
    
    delete task;
  }
  return NULL;
}

int ThreadPool::add_task(Task* task)
{
  m_task_mutex.lock();

  m_tasks.push_back(task);

  m_task_cond_var.signal(); 

  m_task_mutex.unlock();

  return 0;
}
