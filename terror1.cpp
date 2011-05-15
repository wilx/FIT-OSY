#ifndef __PROGTEST__
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iomanip>

#define MAX_TERRORISTS   32

typedef struct TImage
{
    int              m_W;
    int              m_H;
    unsigned char ** m_Data;
} TIMAGE;

void  TerroristHunter    (
    int        databaseSize,
    TIMAGE  ** database,
    int        threads,
    TIMAGE *(* scanner) ( void ),
    void    (* officer) ( TIMAGE *, int ) );

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
    signal ()
    {
        int ret = pthread_cond_signal (&cond);
        if (ret != 0)
            abort ();
    }

    void
    broadcast ()
    {
        int ret = pthread_cond_broadcast (&cond);
        if (ret != 0)
            abort ();
    }


    mutable pthread_cond_t cond;
};



struct AnalystWorkItem
{
    TIMAGE * img;
    unsigned db_index;
};


struct CollectorWorkItem
{
    TIMAGE * img;
    unsigned mask;
};


struct CollectorRecord
{
    TIMAGE * img;
    unsigned mask;
    unsigned checks;
};


enum Constants
{
    QUEUE_LEN = MAX_TERRORISTS * 10
};


/** Terrorist database size.  */
unsigned db_size;
//! The terrorists database itself.
TIMAGE * * db;


/** Queue of AnalystWorkItem instances. */
typedef FixedQueue<AnalystWorkItem, QUEUE_LEN> AnalystQueueType;
AnalystQueueType analyst_queue;

/** Queue analyst_queue mutex. */
PthreadMutex * analyst_qmtx;

/** Condition variable for consumers to wait on when the queue is empty. */
PthreadCond * analyst_qcond_consumer;

/** Condition variable for producer to wait on when the queue is full. */
PthreadCond * analyst_qcond_producer;


//! Mutex for the two counters below.
PthreadMutex * counters_mtx;

//! Number of created work items.
unsigned created_items = 0;

//! Number of analyzed work items.
unsigned analyzed_items = 0;

//! Analyzed work items bookkeeping.
typedef vec<CollectorRecord> CollectorRecordsVec;
CollectorRecordsVec records;


//!
void (* officer_func) ( TIMAGE *, int );


/** A flag for consumers indicating that they should exit.  */
bool end_threads = false;


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


bool
match_pics (TIMAGE const & haystack, unsigned row, unsigned col,
    TIMAGE const & needle)
{
    xassert (row < static_cast<unsigned>(haystack.m_H));
    xassert (col < static_cast<unsigned>(haystack.m_W));
    xassert (row + needle.m_H <= static_cast<unsigned>(haystack.m_H));
    xassert (col + needle.m_W <= static_cast<unsigned>(haystack.m_W));

    bool found = true;
    for (unsigned i = 0; i != static_cast<unsigned>(needle.m_H); ++i)
    {
        for (unsigned j = 0; j != static_cast<unsigned>(needle.m_W); ++j)
        {
            unsigned char needle_pix = needle.m_Data[i][j];
            if (needle_pix == 255)
                continue;
            else if (needle_pix != haystack.m_Data[row + i][col + j])
            {
                found = false;
                goto out;
            }
                
        }
    }

out:
    return found;
}


bool
find_terrorist (TIMAGE * img, unsigned db_index)
{
    TIMAGE * terrorist = db[db_index];
    xassert (terrorist);

    if (terrorist->m_W > img->m_W
        || terrorist->m_H > img->m_H)
        return false;

    bool found = false;
    for (unsigned r = 0; r <= static_cast<unsigned>(img->m_H - terrorist->m_H); ++r)
    {
        for (unsigned c = 0; c <= static_cast<unsigned>(img->m_W - terrorist->m_W); ++c)
        {
            found = match_pics (*img, r, c, *terrorist);
            if (found)
                goto out;
        }
    }

out:
    return found;
}


void *
analyst_thread (void *)
{
    AnalystWorkItem item;

    while (analyst_get_work (&item))
    {
        log ("analyst got work i: " << item.db_index << " " << item.img << std::endl);

        bool found = find_terrorist (item.img, item.db_index);
        unsigned mask = static_cast<unsigned>(found) << item.db_index;

        counters_mtx->lock ();

        ++analyzed_items;
        
        CollectorRecordsVec::iterator end = records.end ();
        CollectorRecordsVec::iterator it, empty_slot = end;
        for (it = records.begin (); it != end; ++it)
            if (! it->img)
                empty_slot = it;
            else if (it->img == item.img)
                break;

        if (it != end)
        {
            it->mask |= mask;
            ++it->checks;
        }
        else if (empty_slot != end)
        {
            empty_slot->img = item.img;
            empty_slot->mask = mask;
            empty_slot->checks = 1;
            it = empty_slot;
        }
        else
        {
            CollectorRecord rec;
            rec.img = item.img;
            rec.mask = mask;
            rec.checks = 1;
            records.push_back (rec);
            it = records.end () - 1;
        }


        if (it->checks == db_size)
        {
            TIMAGE * img = it->img;
            it->img = 0;
            mask = it->mask;
            counters_mtx->unlock ();

            officer_func (img, mask);
        }
        else
            counters_mtx->unlock ();
    }

    return 0;
}


template <typename T>
void
xdelete (T * & p)
{
    delete p;
    p = 0;
}


void
TerroristHunter (
    int        databaseSize,
    TIMAGE  ** database,
    int        thread_count,
    TIMAGE *(* scanner) ( void ),
    void    (* officer) ( TIMAGE *, int ))
{
    int i, ret;
    pthread_t * pthreads;

    analyst_qmtx = new PthreadMutex;
    analyst_qcond_producer = new PthreadCond;
    analyst_qcond_consumer = new PthreadCond;
    analyst_queue.clear ();

    officer_func = officer;
    db_size = databaseSize;
    db = database;

    counters_mtx = new PthreadMutex;
    created_items = 0;
    analyzed_items = 0;

    // Create thread_count analyst threads and one collector thread.

    pthreads = (pthread_t *)calloc (thread_count, sizeof (pthread_t));
    if (pthreads == NULL)
        abort ();

    for (i = 0; i != thread_count; ++i)
    {
        ret = pthread_create (&pthreads[i], NULL, analyst_thread, NULL);
        if (ret != 0)
            abort ();
    }

    TIMAGE * img;
    do
    {
        img = scanner ();
        analyst_insert_work (img);
    }        
    while (img);

    for (i = 0; i != thread_count; ++i)
    {
        ret = pthread_join (pthreads[i], NULL);
        if (ret != 0)
            abort ();
    }

    free (pthreads);

    xdelete (analyst_qmtx);
    xdelete (analyst_qcond_consumer);
    xdelete (analyst_qcond_producer);

    xdelete (counters_mtx);
}





#ifndef __PROGTEST__
unsigned char t0r0[] = { 255, 255, 255, 255, 255, 255, 255, 255 };
unsigned char t0r1[] = { 255,  50,  60,  70, 255, 255, 255, 255 };
unsigned char t0r2[] = { 255,  50,  60,  70, 255, 255, 255, 255 };
unsigned char t0r3[] = { 255, 255,  50,  60,  70,  80, 255, 255 };
unsigned char t0r4[] = { 255, 255,  50,  60,  70,  80, 255, 255 };
unsigned char t0r5[] = { 255, 255, 255, 255, 255, 255, 255, 255 };
unsigned char t0r6[] = { 255, 255, 255, 255, 255, 255, 255, 255 };
unsigned char * t0all[] = { t0r0, t0r1, t0r2, t0r3, t0r4, t0r5, t0r6 };
TIMAGE  t0 = { 8, 7, t0all };  /* one sample */

unsigned char t1r0[] = {  10,  20,  30 };
unsigned char t1r1[] = {  20,  50, 255 };
unsigned char t1r2[] = {  80,  50, 255 };
unsigned char * t1all[] = { t1r0, t1r1, t1r2 };
TIMAGE  t1 = { 3, 3, t1all };  /* another sample */

TIMAGE * terrorists[2] = { &t0, &t1 };

unsigned char i0r0[] = {   1,   2,   3,   4,   5,   6,   7,   8 };
unsigned char i0r1[] = {   2,  50,  60,  70,   6,   7,   8,   1 };
unsigned char i0r2[] = {   3,  50,  60,  70,   7,  10,  20,  30 };
unsigned char i0r3[] = {   4,  50,  60,  70,   8,  20,  50,   3 };
unsigned char i0r4[] = {   5,   6,  50,  60,  70,  80,  50,   4 };
unsigned char i0r5[] = {   6,   7,  50,  60,  70,  80,   4,   5 };
unsigned char i0r6[] = {   7,   8,   1,   2,   3,   4,   5,   6 };
unsigned char i0r7[] = {   8,   1,   2,   3,   4,   5,   6,   7 };
unsigned char * i0all[] = { i0r0, i0r1, i0r2, i0r3, i0r4, i0r5, i0r6, i0r7 };
TIMAGE i0 = { 8, 8, i0all }; /* t0 and t1 here */

unsigned char i1r0[] = {   1,   2,   3,   4,   5,   6,   7,   8 };
unsigned char i1r1[] = {   2,   3,   4,   5,   6,   7,   8,   1 };
unsigned char i1r2[] = {   3,   4,   5,   6,   7,   8,   1,   2 };
unsigned char i1r3[] = {  10,  20,  30,   7,   8,   1,   2,   3 };
unsigned char i1r4[] = {  20,  50,   7,   8,   1,   2,   3,   4 };
unsigned char i1r5[] = {  80,  50,   8,   1,   2,   3,   4,   5 };
unsigned char i1r6[] = {   7,   8,   1,   2,   3,   4,   5,   6 };
unsigned char * i1all[] = { i1r0, i1r1, i1r2, i1r3, i1r4, i1r5, i1r6 };
TIMAGE i1 = { 8, 7, i1all }; /* t1 here */

unsigned char i2r0[] = {   1,  50,  60,  70,   5,   6 };
unsigned char i2r1[] = {   2,  50,  60,  70,   6,   7 };
unsigned char i2r2[] = {   3,   4,  50,  60,  70,  80 };
unsigned char i2r3[] = {  10,  20,  50,  50,  70,  81 };
unsigned char i2r4[] = {  20,  50,   7,   8,   1,   2 };
unsigned char * i2all[] = { i2r0, i2r1, i2r2, i2r3, i2r4 };
TIMAGE i2 = { 6, 5, i2all }; /* no terrorists here */

TIMAGE * scans[3] = { &i0, &i1, &i2 };

TIMAGE   * dummyScanner ( void )
{
    static int idx = 0;

    if ( idx != 10000 )
    {
        return ( scans[idx++ % 3] );
    }
    else
        return ( NULL );
}

void       dummyOfficer  ( TIMAGE * img, int found )
{
    int i;

    printf ( "Image: %d x %d  ", img -> m_W, img -> m_H );
    if ( found )
    {
        printf ( "TERRORISTS:" );
        for ( i = 0; i < (int)sizeof ( found ) * 8; i ++ )
            if ( found & ( 1 << i ) )
                printf ( " %d", i );
        printf ( "\n" );
    }
    else
        printf ( "no terrorists found\n" );
}

int main ()
{
    xassert (match_pics (i1, 3, 0, t1));

    TerroristHunter ( 2, terrorists, 2, dummyScanner, dummyOfficer );
    return ( 0 );
}
#endif /* __PROGTEST__ */
