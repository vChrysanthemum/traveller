#include "core/stack.h"
#include "core/zmalloc.h"

void stackClean(stack *stack) {
    if (0 != stack->free) {
        for(int i = 0; i < stack->len; i++) {
            stack->free(stack->data[i]);
        }
    }
    stack->len = 0;
}

void stackRelease(stack *stack) {
    stackClean(stack);
    zfree(stack->data);
}

stack* stackCreate(int initialCapacity) {
    stack *s = (stack*)zmalloc(sizeof(stack));
    s->len = 0;
    s->cap = initialCapacity;
    s->data = (void**)zmalloc(sizeof(void*) * s->cap);
    s->free = 0;
    return s;
}

void* stackPop(stack* stack) {
    stack->len--;
    return stack->data[stack->len];
}

void stackPush(stack *stack, void *data) {
    if (stack->len == stack->cap) {
        stack->cap *= 2;
        stack->data = (void**)zrealloc(stack->data, sizeof(void*) * stack->cap);
    }
    stack->len++;
    stack->data[stack->len-1] = data;
}
