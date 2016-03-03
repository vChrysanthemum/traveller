#ifndef __CORE_STACK_H__
#define __CORE_STACK_H__

typedef struct stack {
    void **data;
    int len;
    int cap;
    void (*free)(void*);
} stack;

void stackClean(stack *stack);
void stackRelease(stack *stack);
stack* stackCreate(int initialCapacity);
void* stackPop(stack *stack);
void stackPush(stack *stack, void *data);

#endif
