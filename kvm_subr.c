#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/atomic.h>
#include <sys/spl.h>
#include <sys/cpuvar.h>
#include <sys/segments.h>

#include "msr.h"
#include "vmx.h"
#include "irqflags.h"
#include "iodev.h"
#include "kvm_host.h"
#include "kvm_x86host.h"
#include "kvm.h"

unsigned long
kvm_dirty_bitmap_bytes(struct kvm_memory_slot *memslot)
{
	/* XXX */
	/* 	return ALIGN(memslot->npages, BITS_PER_LONG) / 8; */
	return ((BT_BITOUL(memslot->npages)) / 8);
}

struct kvm_vcpu *
kvm_get_vcpu(struct kvm *kvm, int i)
{
#ifdef XXX
	smp_rmb();
#endif /*XXX*/
	return kvm->vcpus[i];
}

void
kvm_fx_save(struct i387_fxsave_struct *image)
{
	__asm__("fxsave (%0)":: "r" (image));
}

void
kvm_fx_restore(struct i387_fxsave_struct *image)
{
	__asm__("fxrstor (%0)":: "r" (image));
}

void
kvm_fx_finit(void)
{
	__asm__("finit");
}

uint32_t
get_rdx_init_val(void)
{
	return 0x600; /* P6 family */
}

void
kvm_inject_gp(struct kvm_vcpu *vcpu, uint32_t error_code)
{
	kvm_queue_exception_e(vcpu, GP_VECTOR, error_code);
}

unsigned long long
native_read_tscp(unsigned int *aux)
{
	unsigned long low, high;
	__asm__ volatile(".byte 0x0f,0x01,0xf9"
		     : "=a" (low), "=d" (high), "=c" (*aux));
	return low | ((uint64_t)high << 32);
}

unsigned long long
native_read_msr(unsigned int msr)
{
	DECLARE_ARGS(val, low, high);

	__asm__ volatile("rdmsr" : EAX_EDX_RET(val, low, high) : "c" (msr));
	return EAX_EDX_VAL(val, low, high);
}

void
native_write_msr(unsigned int msr, unsigned low, unsigned high)
{
	__asm__ volatile("wrmsr" : : "c" (msr),
	    "a"(low), "d" (high) : "memory");
}

unsigned long long
__native_read_tsc(void)
{
	DECLARE_ARGS(val, low, high);

	__asm__ volatile("rdtsc" : EAX_EDX_RET(val, low, high));

	return EAX_EDX_VAL(val, low, high);
}

unsigned long long
native_read_pmc(int counter)
{
	DECLARE_ARGS(val, low, high);

	__asm__ volatile("rdpmc" : EAX_EDX_RET(val, low, high) : "c" (counter));
	return EAX_EDX_VAL(val, low, high);
}

#ifdef XXX_KVM_DOESNOTLINK
void wrmsr(unsigned msr, unsigned low, unsigned high)
{
	native_write_msr(msr, low, high);
}
#endif /* XXX_KVM_DOESNOTLINK*/

int
wrmsr_safe(unsigned msr, unsigned low, unsigned high)
{
	return (native_write_msr_safe(msr, low, high));
}

int
rdmsrl_safe(unsigned msr, unsigned long long *p)
{
	int err;

	*p = native_read_msr_safe(msr, &err);
	return (err);
}

int
rdmsr_on_cpu(unsigned int cpu, uint32_t msr_no, uint32_t *l, uint32_t *h)
{
	rdmsr(msr_no, *l, *h);
	return 0;
}

int
wrmsr_on_cpu(unsigned int cpu, uint32_t msr_no, uint32_t l, uint32_t h)
{
	wrmsr(msr_no, l, h);
	return 0;
}

unsigned long read_msr(unsigned long msr)
{
	uint64_t value;

	rdmsrl(msr, value);
	return (value);
}

unsigned long kvm_read_tr_base(void)
{
	unsigned short tr;
	__asm__("str %0" : "=g"(tr));
	return segment_base(tr);
}

