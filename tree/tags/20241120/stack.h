/*
 *  STACK.H - Stack management functions.
 *  Written 1995,96 by Andrew Clarke and released to the public domain.
 */

#ifndef __STACK_H__
#define __STACK_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct stacknode 
{
    struct stacknode *p_prev;
    void *p_item;
} STACKNODE;

typedef struct 
{
    STACKNODE *p_top;
    unsigned long items;
} STACK;

void stackDefaults(STACK * p_stack);
void stackInit(STACK * p_stack);
void stackTerm(STACK * p_stack);
int stackPushItem(STACK * p_stack, void *p_item);
void *stackPopItem(STACK * p_stack);
unsigned long stackTotalItems(STACK * p_stack);
int stackIsEmpty(STACK * p_stack);


#ifdef __cplusplus
}
#endif

#endif

