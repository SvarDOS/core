/*
 *  STACK.C - Stack management functions.
 *  Written 1995,96 by Andrew Clarke and released to the public domain.
 *
 *  Slightly modified for inclusion in pdTree
 *  by Jeremy Davis <jeremyd@computer.org> Jan. 2001
 */

#include <stdlib.h>  // for malloc
#include "stack.h"

// initialize stack structure to known initial values.
void stackDefaults(STACK * p_stack)
{
    p_stack->p_top = NULL;
    p_stack->items = 0L;
}

#if defined __BORLANDC__ || __TURBOC__
#pragma argsused
#endif
// place any extra stack initialization here
void stackInit(STACK * p_stack)
{
    (void)p_stack;
}

// ensures stack is empty and all allocated memory freed
void stackTerm(STACK * p_stack)
{
    while (!stackIsEmpty(p_stack)) 
    {
        stackPopItem(p_stack);
    }
}

// add an item to the stack, allocating memory for STACKNODE
int stackPushItem(STACK * p_stack, void *p_item)
{
    STACKNODE *p_node;
    p_node = malloc(sizeof(STACKNODE));
    if (!p_node)
        return 0;
    p_node->p_item = p_item;
    if (!p_stack->p_top)
        p_node->p_prev = NULL;
    else
        p_node->p_prev = p_stack->p_top;
    p_stack->p_top = p_node;
    p_stack->items++;
    return 1;
}

// removes item from the stack, freeing memory allocated for STACKNODE
void *stackPopItem(STACK * p_stack)
{
    void *p_item;
    STACKNODE *p_node;
    if (!p_stack->items)
        return NULL;
    p_item = p_stack->p_top->p_item;
    p_node = p_stack->p_top;
    p_stack->p_top = p_stack->p_top->p_prev;
    free(p_node);
    p_stack->items--;
    return p_item;
}

// returns the current size of (items on) the stack
unsigned long stackTotalItems(STACK * p_stack)
{
    return p_stack->items;
}

// returns true if the stack is empty, nonzero otherwise
int stackIsEmpty(STACK * p_stack)
{
    return p_stack->items == 0;
}



/*
 *  The rest of this file is a sample program to show how
 *  to use the stack functions and also to test the stack functions.
 *
 */

#ifdef TEST_STACK

#include <stdio.h>

int main(void)
{
    STACK p_stack;
    char *str;

    stackDefaults(&p_stack);
    stackInit(&p_stack);
    stackPushItem(&p_stack, "One banana");
    stackPushItem(&p_stack, "Two banana");
    stackPushItem(&p_stack, "Three banana");
    stackPushItem(&p_stack, "Four banana");
    stackPushItem(&p_stack, "Five banana");

    str = stackPopItem(&p_stack);
    while (str) 
    {
        printf("%s\n", str);
        str = stackPopItem(&p_stack);
    }
    stackTerm(&p_stack);

    return 0;
}

#endif

