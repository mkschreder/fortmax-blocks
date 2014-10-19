#pragma once

#include <kernel/object.h>
#include <kernel/list.h>
#include <stdint.h>
#include <stddef.h>

struct kvar {
	const char *name;
	uint16_t id; 
	struct list_head list; 
};

struct kvar *kvar_get_all(void); 
struct kvar *kvar_register(struct kvar *var);
struct kvar *kvar_find(const char *name);

uint32_t hash(const char *str);

#define kvar_declare(T, N) \
	struct T##N##_s {\
		struct kvar var;\
		T value;\
		T prev;\
	}__attribute__((packed));\
	static struct T##N##_s T##N##_s_inst = {\
		.var.name = #T "_" #N,\
		.var.list.next = 0,\
		.var.list.prev = 0\
	};\
	static struct T##N##_s *N = &T##N##_s_inst;\
	static void __attribute((constructor)) N##_register_var(void){ \
		struct kvar *v = kvar_register(&N->var); \
		if(v != &N->var)\
			N = container_of(v, struct T##N##_s, var); \
	}\

#define kvar_import(T, N) \
	struct T##N##_s {\
		struct kvar var;\
		T value;\
		T prev;\
	} __attribute__((packed));\
	static struct T##N##_s *N = 0; {\
		struct kvar *v = kvar_find(#T "_" #N); \
		if(v)\
			N = container_of(v, struct T##N##_s, var); \
	};\
	
#define kvar_get(name) (name->value)

#define kvar_set(name, val) { name->prev = name->value; name->value = val; }
