#pragma once
#include <smp_utils>

struct struct_spinny {
	smp_spinlock memory;
};
extern struct_spinny mr_spinny;
