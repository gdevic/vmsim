/******************************************************************************
*                                                                             *
*   Module:     Queue.h                                                       *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       8/29/97                                                       *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file for the queue functions.

    Usage:

    Create Data Structure
    ---------------------

            q = QCreateAlloc()
            if q==NULL then Fail

                or

            TQueue q
            QCreate( &q )

    Destruct Data Structure
    -----------------------

            QDestroyAlloc( &q )

                or

            QDestroy( &q )

    Stack Operations
    ----------------

            fResult = QPush( &q, element )
            element = QPop( &q )
            fEmpty  = QIsNotEmpty( &q )
            element = QCurrent( &q )   (peek at the next one to pop)

    Linked List Operations
    ----------------------

            fResult = QInsert( &q, element )
            fResult = QAdd( &q, element )
            element = QDelete( &q )
            fEmpty  = QIsNotEmpty( &q )
            fFound  = QFind( &q, fmCmp, const element )

            element = QFirst( &q )     (traverse the doubly linked list)
            element = QLast( &q )
            element = QNext( &q )
            element = QPrev( &q )
            element = QCurrent( &q )

    Queue Operations
    ---------------------

            fResult = QEnqueue( &q, element )
            element = QDequeue( &q )
            fEmpty  = QIsNotEmpty( &q )
            element = QCurrent( &q )   (peek at the next one to dequeue)

    Sorted List / Priority Queue Operations
    ---------------------------------------

            fResult = QSort( &q )

            fResult = QPriorityEnqueue( &q, fmCmp, element )
            element = QDequeue( &q )
            fEmpty  = QIsNotEmpty( &q )
            element = QCurrent( &q )   (peek at the next one to dequeue)

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 8/29/97    Original                                             Goran Devic *
* 3/08/00    Modified for NT kernel driver use                    Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _QUEUE_H_
#define _QUEUE_H_

/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/

#include <ntddk.h>                  // Include NT ddk header file

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

typedef struct TNodePtr pTNode;

typedef struct TNodePtr
{
    void * p;                           // Pointer to the data element
    pTNode *pPrev, *pNext;              // Doubly linked list pointers

} TNode;

typedef struct
{
    TNode *pHead, *pTail, *pCur;        // Queue pointers
    unsigned int nSize;                 // Number of nodes in a queue
    POOL_TYPE PoolType;                 // Pool type allocation (for nodes)
    KSPIN_LOCK SpinLock;                // Serializes access to data

} TQueue;


/******************************************************************************
*                                                                             *
*   External Functions                                                        *
*                                                                             *
******************************************************************************/

// Basic functions provided by the Queue module

extern int    QCheck( IN OUT TQueue *q );

extern void   QCreate( IN POOL_TYPE PoolType, IN OUT TQueue *q );
extern void   QDestroy( IN TQueue *q );

extern TQueue * QCreateAlloc( IN POOL_TYPE PoolType );
extern void   QDestroyAlloc( IN OUT TQueue *q );


extern int    QPush( IN OUT TQueue * q, IN void * p );
extern void * QPop( IN OUT TQueue * q );

extern int    QInsert( IN OUT TQueue * q, IN void * p );
extern int    QAdd( IN OUT TQueue * q, IN void * p );
extern void * QDelete( IN OUT TQueue * q );

extern void * QFirst( IN OUT TQueue * q );
extern void * QLast( IN OUT TQueue * q );
extern void * QCurrent( IN OUT TQueue * q );
extern void * QNext( IN OUT TQueue * q );
extern void * QPrev( IN OUT TQueue * q );

extern int    QIsNotEmpty( IN TQueue * q );
extern int    QFind( IN OUT TQueue * q, IN int fnCmp( void *p1, const void *p2), const void * p);

extern int    QEnqueue( IN OUT TQueue * q, IN void * p );
extern void * QDequeue( IN OUT TQueue * q );
extern int    QPriorityEnqueue( IN OUT TQueue * q, IN int fnCmp( void *p1, void *p2), void * p);
extern int    QSort( IN OUT TQueue * q, IN int fnCmp( void *p1, void *p2));


#endif //  _QUEUE_H_
