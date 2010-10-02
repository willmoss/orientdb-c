#include <setjmp.h>
#include "o_exceptions.h"
#include <string.h>
#include "o_memory.h"

struct jmp_stack
{
	struct __jmp_buf_tag * cur_jmp;
	struct jmp_stack * back;
};

struct exception
{
	char * name;
	void * instance;
};

__thread struct exception cur_exception;
__thread struct jmp_stack *stack_top = 0;

int o_catch_type(char * name, void **val, jmp_buf cur_jmp)
{
	if (cur_jmp == stack_top->cur_jmp)
	{
		struct jmp_stack *stack_free = stack_top;
		stack_top = stack_top->back;
		o_free(stack_free);
	}
	if (cur_exception.name == 0)
	{
		if (o_exception_typeof(cur_exception.instance, name))
		{
			*val = cur_exception.instance;
			return 1;
		}
		return 0;
	}
	else if (strcmp(cur_exception.name, name) == 0)
	{
		*val = cur_exception.instance;
		return 1;
	}
	else
		return 0;
}

int o_try_push(jmp_buf jump)
{
	struct jmp_stack * to_push = o_malloc(sizeof(struct jmp_stack));
	//to_push->cur_jmp = cur_jmp;
	to_push->cur_jmp = jump;
	to_push->back = stack_top;
	return 1;
}

void o_notify_object(char * name, void * instance)
{
	cur_exception.name = name;
	cur_exception.instance = instance;
	longjmp(stack_top->cur_jmp, 1);
}

void o_notify_exception(struct o_exception * exception)
{
	cur_exception.name = 0;
	cur_exception.instance = exception;
	longjmp(stack_top->cur_jmp, 1);
}

