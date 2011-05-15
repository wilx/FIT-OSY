#ifndef __PROGTEST__
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iomanip>

#define AGENDA_ID              0x01
#define AGENDA_CAR             0x02
#define AGENDA_TAX             0x04
#define AGENDA_LIFTBOY         0x80

typedef struct TRequestID
{
    char                      m_Data[100];
} TREQUESTID;

typedef struct TRequestCar
{
    int                       m_Data[80];
} TREQUESTCAR;

typedef struct TRequestTax
{
    float                     m_Data[120];
} TREQUESTTAX;

typedef union URequest
{
    TREQUESTID                m_ID;
    TREQUESTCAR               m_Car;
    TREQUESTTAX               m_Tax;
} UREQUEST;

typedef struct TCitizen
{
    int                       m_Agenda; /* one of AGENDA_ID/CAR/TAX */
    UREQUEST                  m_Req;
} TCITIZEN;

typedef struct TOffice
{
    TCITIZEN             * (* m_CitizenGen)                 ( void );
    void                   (* m_CitizenDone)                ( TCITIZEN        * citizen,
        int               status );

    int                    (* m_ProcessID)                  ( TREQUESTID      * data );
    int                    (* m_ProcessCar)                 ( TREQUESTCAR     * data );
    int                    (* m_ProcessTax)                 ( TREQUESTTAX     * data );

    int                    (* m_Register)                   ( int               agenda );
    void                   (* m_Deregister)                 ( void );

    int                       m_ClerkNr;
    int                     * m_ClerkAgenda;    /* any combination of AGENDA_ID/CAR/TAX, m_ClerkNr long */
} TOFFICE;


#define xassert(X) assert(X)
//#define log(X) do { std::cerr << X; } while (0)
#define log(X) /*empty*/

#else
#define xassert(X) /*empty*/
#define log(X) /*empty*/

#endif /* __PROGTEST__ */


template <typename T>
void
xswap (T & a, T & b)
{
    T tmp (a);
    a = b;
    b = tmp;
}


template <typename T>
class vec
{
public:
    typedef T value_type;
    typedef T * iterator;
    typedef T const * const_iterator;
    typedef T & reference;
    typedef T const & const_reference;
    typedef unsigned size_type;

    vec ()
    {
        init (1);
    }


    vec (vec const & other)
    {
        unsigned other_size = other.size ();
        init (other_size);

        buf_last = buf + other_size;

        const_iterator other_it = other.begin ();
        for (iterator it = begin (); it != end (); ++it, ++other_it)
            *it = *other_it;
    }


    explicit
    vec (unsigned len)
    {
        init (len == 0 ? 1 : len);
        resize (len);
    }


    vec (unsigned len, T const & val)
    {
        init (len == 0 ? 1 : len);
        buf_last = buf + len;
        iterator end_it = end ();
        for (iterator it = begin (); it != end_it; ++it)
            *it = val;
    }


    vec (const_iterator b, const_iterator e)
    {
        unsigned len = e - b;
        init (len == 0 ? 1 : len);
        buf_last = buf + len;
        iterator end_it = end ();
        for (iterator it = begin (); it != end_it; ++it, ++b)
            *it = *b;
    }


    vec &
    operator = (vec const & other)
    {
        vec (other).swap (*this);
        return *this;
    }


    ~vec ()
    {
        free (buf);
    }


    iterator
    begin ()
    {
        return buf;
    }


    iterator
    end ()
    {
        return buf_last;
    }


    const_iterator
    begin () const
    {
        return buf;
    }


    const_iterator
    end () const
    {
        return buf_last;
    }


    reference
    operator [] (unsigned i)
    {
        return buf[i];
    }


    const_reference
    operator [] (unsigned i) const
    {
        return buf[i];
    }


    unsigned
    size () const
    {
        return static_cast<size_type>(buf_last - buf);
    }


    unsigned
    capacity () const
    {
        return static_cast<size_type>(buf_end - buf);
    }


    void
    resize (size_type new_size, T const & val = T ())
    {
        size_type old_size = size ();
        if (new_size > old_size)
        {
            reserve (new_size);

            buf_last = buf + new_size;

            for (iterator it = begin () + old_size; it != end (); ++it)
                *it = val;
        }
        else
            buf_last = buf + new_size;
    }


    void
    reserve (size_type new_cap)
    {
        size_type old_size = size ();
        size_type old_cap = capacity ();
        if (new_cap <= old_cap)
            return;

        value_type * new_buf = vec_realloc (buf, new_cap);
        if (! new_buf)
            throw "realloc";

        buf = new_buf;
        buf_last = buf + old_size;
        buf_end = buf + new_cap;
    }


    void
    push_back (T const & x)
    {
        size_type const old_size = size ();
        resize (old_size + 1);
        buf[old_size] = x;
    }


    void
    swap (vec & other)
    {
        xswap (buf, other.buf);
        xswap (buf_last, other.buf_last);
        xswap (buf_end, other.buf_end);
    }


private:
    void
    init (size_type cap)
    {
        buf = vec_malloc (cap);
        buf_last = buf;
        buf_end = buf + cap;
    }


    static
    value_type *
    vec_malloc (size_type count)
    {
        return static_cast<value_type *>(std::malloc (count * sizeof (value_type)));
    }


    static
    value_type *
    vec_realloc (value_type * buf, size_type count)
    {
        value_type * new_buf
            = static_cast<value_type *>(
                realloc (buf, count * sizeof (value_type)));

        return new_buf;
    }


    value_type * buf;
    value_type * buf_last;
    value_type * buf_end;
};


template <typename T, unsigned N>
class FixedQueue
{
public:
    typedef T value_type;

    FixedQueue ()
        : head (0)
        , tail (0)
        , size (0)
    {
        T const val = T ();
        for (unsigned i = 0; i != N; ++i)
            queue[i] = val;
    }

    unsigned
    get_size () const
    {
        return size;
    }

    unsigned
    get_capacity () const
    {
        return N;
    }

    bool
    full () const
    {
        return size != 0 && head == tail;
    }

    bool
    empty () const
    {
        return size == 0;
    }

    value_type &
    peek_front ()
    {
        return queue[tail];
    }

    value_type const &
    peek_front () const
    {
        return queue[tail];
    }

    void
    pop_front ()
    {
        xassert (! empty ());

        --size;
        tail = (tail + 1) % N;
    }

    void
    push_back (value_type const & item)
    {
        xassert (! full ());

        queue[head] = item;
        head = (head + 1) % N;
        ++size;
    }

    void
    clear ()
    {
        size = 0;
        head = 0;
        tail = 0;
    }

private:
    value_type queue[N];
    unsigned head;
    unsigned tail;
    unsigned size;
};


struct PthreadMutex
{
    PthreadMutex ()
    {
        int ret = pthread_mutex_init (&mtx, NULL);
        if (ret != 0)
            abort ();
    }

    ~PthreadMutex ()
    {
        int ret = pthread_mutex_destroy (&mtx);
        if (ret != 0)
            abort ();
    }

    void
    lock () const
    {
        int ret = pthread_mutex_lock (&mtx);
        if (ret != 0)
            abort ();
    }

    void
    unlock () const
    {
        int ret = pthread_mutex_unlock (&mtx);
        if (ret != 0)
            abort ();
    }

    mutable pthread_mutex_t mtx;
};


struct PthreadCond
{
    PthreadCond ()
    {
        int ret = pthread_cond_init (&cond, NULL);
        if (ret != 0)
            abort ();
    }

    ~PthreadCond ()
    {
        int ret = pthread_cond_destroy (&cond);
        if (ret != 0)
            abort ();
    }

    void
    wait (PthreadMutex const & mtx) const
    {
        int ret = pthread_cond_wait (&cond, &mtx.mtx);
        if (ret != 0)
            abort ();
    }

    void
    signal () const
    {
        int ret = pthread_cond_signal (&cond);
        if (ret != 0)
            abort ();
    }

    void
    broadcast () const
    {
        int ret = pthread_cond_broadcast (&cond);
        if (ret != 0)
            abort ();
    }


    mutable pthread_cond_t cond;
};


enum Constants
{
    QUEUE_LEN = 100
};


typedef FixedQueue<TRequestID, QUEUE_LEN> IdRequestQueueType;
typedef FixedQueue<TRequestCar, QUEUE_LEN> CarRequestQueueType;
typedef FixedQueue<TRequestTax, QUEUE_LEN> TaxRequestQueueType;

typedef FixedQueue<TCitizen, QUEUE_LEN> CitizenRequestQueueType;


struct Office
{
    Office (TOffice & o)
        : office (o)
        , created_items (0)
        , analyzed_items (0)
        , end_threads (false)
    { }

    // Office parameter.
    TOffice & office;

    //! Queue mutex. */
    PthreadMutex qmtx;

    //! Condition variable for queue state change. */
    PthreadCond qcond;

    //! Number of created work items.
    unsigned created_items;

    //! Number of analyzed work items.
    unsigned analyzed_items;

    //! A flag for consumers indicating that they should exit.
    bool end_threads;

    //!
    CitizenRequestQueueType id_queue;

    //!
    CitizenRequestQueueType car_queue;

    //!
    CitizenRequestQueueType tax_queue;
};


struct ThreadParam
{
    ThreadParam (Office & o, int a)
        : office (o)
        , agenda (a)
    { }

    Office & office;
    int agenda;
};


#if 0
void
analyst_insert_work (TIMAGE * img)
{
    /* Lock the queue.  */

    analyst_qmtx->lock ();

    if (! img)
    {
        end_threads = true;
        analyst_qmtx->unlock ();
        analyst_qcond_consumer->broadcast ();
        return;
    }

    // Insert all combinations of image and DB index into analysts'
    // queue.

    for (unsigned i = 0; i != db_size; ++i)
    {
        log ("inserting work i:" << i << " " << img << std::endl);

        /* Wait for non-full queue.  */

        while (analyst_queue.full ())
            analyst_qcond_producer->wait (*analyst_qmtx);

        /* Insert item into the queue.  */

        AnalystWorkItem item;
        item.img = img;
        item.db_index = i;
        analyst_queue.push_back (item);

        counters_mtx->lock ();
        ++created_items;
        counters_mtx->unlock ();
    }

    analyst_qmtx->unlock ();
    analyst_qcond_consumer->signal ();
}


//! Gets work for analysts. This function is called only by analysts.
bool
analyst_get_work (AnalystWorkItem * item)
{
    /* Lock the queue.  */

    analyst_qmtx->lock ();

    if (end_threads && analyst_queue.empty ())
    {
        analyst_qmtx->unlock ();
        return false;
    }

    /* Wait for non-empty queue or end of work.  */

    while (analyst_queue.empty () && ! end_threads)
        analyst_qcond_consumer->wait (*analyst_qmtx);

    if (end_threads && analyst_queue.empty ())
    {
        analyst_qmtx->unlock ();
        return false;
    }

    // Remember if the queue was full.

    bool was_full = analyst_queue.full ();

    /* Remove work item from the queue.   */

    *item = analyst_queue.peek_front ();
    analyst_queue.pop_front ();

    /* Unlock the queue.  */

    analyst_qmtx->unlock ();

    /* Signal the producer that the queue is not full anymore.  */

    if (was_full)
        analyst_qcond_producer->signal ();

    return true;
}
#endif

void *
clerk_thread (void * param)
{
    xassert (param);
    ThreadParam & tp = *static_cast<ThreadParam *>(param);
    Office & o = tp.office;

    o.office.m_Register (tp.agenda);

    delete &tp;
    return 0;
}


template <typename T>
void
xdelete (T * & p)
{
    delete p;
    p = 0;
}


template <typename Req, unsigned Len>
bool
queue_request (Office & o, FixedQueue<Req, Len> & queue, Req const & req)
{
    o.qmtx.lock ();

    while (queue.full ())
	o.qcond.wait (o.qmtx);

    queue.push_back (req);

    o.qcond.broadcast ();

    o.qmtx.unlock ();
    
    return true;
}


void
queue_request (Office & o, TCitizen * citizen)
{
    // Check for the last request first.

    if (! citizen)
    {
	o.qmtx.lock ();
        o.end_threads = true;
        o.qcond.broadcast ();
        o.qmtx.unlock ();
        return;
    }

    // Pick the right queue for the agenda.
    
    bool ret;
    switch (citizen->m_Agenda)
    {
    case AGENDA_ID:
	ret = queue_request (o, o.id_queue, *citizen);
	break;
	
    case AGENDA_CAR:
	ret = queue_request (o, o.car_queue, *citizen);
	break;
	
    case AGENDA_TAX:
	ret = queue_request (o, o.tax_queue, *citizen);
	break;
	
    default:
	abort ();
    }
    
    o.qmtx.lock ();
    
    o.qmtx.unlock ();
}


int
ThreadedOffice (TOFFICE * o)
{
    int i, ret;
    vec<pthread_t> pthreads;

    Office office (*o);

    // Create thread_count analyst threads and one collector thread.

    pthreads.resize (o->m_ClerkNr);
    for (i = 0; i != o->m_ClerkNr; ++i)
    {
        ThreadParam * tp = new ThreadParam (office, o->m_ClerkAgenda[i]);
        ret = pthread_create (&pthreads[i], NULL, clerk_thread, tp);
        if (ret != 0)
            abort ();
    }

    o->m_Register (AGENDA_LIFTBOY);

    TCitizen * citizen;
    do
    {
        citizen = o->m_CitizenGen ();
        queue_request (office, citizen);
    }        
    while (citizen);

    o->m_Deregister ();

    for (i = 0; i != o->m_ClerkNr; ++i)
    {
        ret = pthread_join (pthreads[i], NULL);
        if (ret != 0)
            abort ();
    }

    return 0;
}





#ifndef __PROGTEST__

int main ()
{
    

    return ( 0 );
}
#endif /* __PROGTEST__ */
