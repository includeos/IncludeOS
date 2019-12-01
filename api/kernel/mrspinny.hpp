#pragma once
#include <smp_utils>

struct struct_spinny {
	spinlock_t memory = 0;
	
};
extern struct_spinny mr_spinny;
