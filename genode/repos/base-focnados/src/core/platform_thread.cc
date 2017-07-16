/*
 * \brief  Fiasco thread facility
 * \author Stefan Kalkowski
 * \date   2011-01-04
 */

/*
 * Copyright (C) 2011-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/ipc.h>
#include <base/log.h>
#include <util/string.h>

/* core includes */
#include <platform_thread.h>
#include <platform.h>
#include <core_env.h>

/* Fiasco includes */
namespace Fiasco {
#include <l4/sys/debugger.h>
#include <l4/sys/factory.h>
#include <l4/sys/irq.h>
#include <l4/sys/scheduler.h>
#include <l4/sys/thread.h>
#include <l4/sys/types.h>
}

using namespace Genode;
using namespace Fiasco;


int Platform_thread::start(void *ip, void *sp)
{
	/* map the pager cap */
	if (_platform_pd)
		_pager.map(_platform_pd->native_task().data()->kcap());

	/* reserve utcb area and associate thread with this task */
	l4_thread_control_start();
	l4_thread_control_pager(_pager.remote);
	l4_thread_control_exc_handler(_pager.remote);
	l4_thread_control_bind((l4_utcb_t *)_utcb, _platform_pd->native_task().data()->kcap());
	l4_msgtag_t tag = l4_thread_control_commit(_thread.local.data()->kcap());
	if (l4_msgtag_has_error(tag)) {
		PWRN("l4_thread_control_commit for %lx failed!",
		     (unsigned long) _thread.local.data()->kcap());
		return -1;
	}
	_state = RUNNING;

	/* set ip and sp and run the thread */
	tag = l4_thread_ex_regs(_thread.local.data()->kcap(), (l4_addr_t) ip,
	                        (l4_addr_t) sp, 0);
	if (l4_msgtag_has_error(tag)) {
		PWRN("l4_thread_ex_regs failed!");
		return -1;
	}

	return 0;
}


void Platform_thread::pause()
{
	if (!_pager_obj)
		return;

	_pager_obj->state.lock.lock();

	if (_pager_obj->state.paused == true) {
		_pager_obj->state.lock.unlock();
		return;
	}

	unsigned exc      = _pager_obj->state.exceptions;
	_pager_obj->state.ip  = ~0UL;
	_pager_obj->state.sp  = ~0UL;
	l4_umword_t flags = L4_THREAD_EX_REGS_TRIGGER_EXCEPTION;

	/* Mark thread to be stopped */
	_pager_obj->state.paused = true;

	/*
	 * Force the thread to be paused to trigger an exception.
	 * The pager thread, which also acts as exception handler, will
	 * leave the thread in exception state until, it gets woken again
	 */
	l4_thread_ex_regs_ret(_thread.local.data()->kcap(), &_pager_obj->state.ip,
	                      &_pager_obj->state.sp, &flags);

	/*
	 * The thread state ("ready") is encoded in the lowest bit of the flags.
	 */
	bool in_syscall = (flags & 1) == 0;
	_pager_obj->state.lock.unlock();

	/**
	 * Check whether the thread was in ongoing ipc, if so it won't raise
	 * an exception before ipc is completed.
	 */
	if (!in_syscall) {
		/*
		 * Wait until the pager thread got an exception from
		 * the requested thread, and stored its thread state
		 */
		while (exc == _pager_obj->state.exceptions && !_pager_obj->state.in_exception)
			l4_thread_switch(_thread.local.data()->kcap());
	}
}


void Platform_thread::single_step(bool enabled)
{
	Fiasco::l4_cap_idx_t const tid = thread().local.data()->kcap();

	enum { THREAD_SINGLE_STEP = 0x40000 };
	int const flags = enabled ? THREAD_SINGLE_STEP : 0;

	Fiasco::l4_thread_ex_regs(tid, ~0UL, ~0UL, flags);
}


void Platform_thread::resume()
{
	if (!_pager_obj)
		return;

	_pager_obj->state.lock.lock();

	/* Mark thread to be runable again */
	_pager_obj->state.paused = false;
	_pager_obj->state.lock.unlock();

	/* Send a message to the exception handler, to unblock the client */
	Msgbuf<16> snd, rcv;
	snd.insert(_pager_obj);
	ipc_call(_pager_obj->cap(), snd, rcv, 0);
}


void Platform_thread::bind(Platform_pd *pd)
{
	_platform_pd = pd;
	_gate.map(pd->native_task().data()->kcap());
	_irq.map(pd->native_task().data()->kcap());
}


void Platform_thread::unbind()
{
	if (_state == RUNNING) {
		/* first set the thread as its own pager */
		l4_thread_control_start();
		l4_thread_control_pager(_gate.remote);
		l4_thread_control_exc_handler(_gate.remote);
		if (l4_msgtag_has_error(l4_thread_control_commit(_thread.local.data()->kcap())))
			PWRN("l4_thread_control_commit for %lx failed!",
				 (unsigned long) _thread.local.data()->kcap());

		/* now force it into a pagefault */
		l4_thread_ex_regs(_thread.local.data()->kcap(), 0, 0, L4_THREAD_EX_REGS_CANCEL);
	}

	_platform_pd = (Platform_pd*) 0;
}


void Platform_thread::pager(Pager_object *pager_obj)
{
	_pager_obj   = pager_obj;
	if (_pager_obj)
		_pager.local = pager_obj->cap();
	else
		_pager.local = Native_capability();
}


void Platform_thread::state(Thread_state s)
{
	if (_pager_obj)
		_pager_obj->state = s;
}


Thread_state Platform_thread::state()
{
	Thread_state s;
	if (_pager_obj) s = _pager_obj->state;

	s.kcap = _gate.remote;
	s.id   = _gate.local.local_name();
	s.utcb = _utcb;

	return s;
}


void Platform_thread::cancel_blocking()
{
	l4_irq_trigger(_irq.local.data()->kcap());
}


void Platform_thread::affinity(Affinity::Location location)
{
	_location = location;

	int const cpu = location.xpos();

	l4_sched_param_t params;
	/* set priority of thread */
	if (_dl<=0)
		params = l4_sched_param_by_type(Fixed_prio, _prio, 0);
	else if(_prio<=0)
		params = l4_sched_param_by_type(Deadline, _dl, 0);
	else{
		PWRN("wrong scheduling type");
		return;
	}

	params.affinity = l4_sched_cpu_set(cpu, 0, 1);
	l4_msgtag_t tag = l4_scheduler_run_thread(L4_BASE_SCHEDULER_CAP,
	                                          _thread.local.data()->kcap(), &params);
	if (l4_error(tag))
		PWRN("setting affinity of %lx to %d failed!", _thread.local.data()->kcap(), cpu);
}


Affinity::Location Platform_thread::affinity() const
{
	return _location;
}


static Rpc_cap_factory &thread_cap_factory()
{
	static Rpc_cap_factory inst(*platform()->core_mem_alloc());
	return inst;
}


void Platform_thread::_create_thread()
{
	l4_msgtag_t tag = l4_factory_create_thread(L4_BASE_FACTORY_CAP,
	                                           _thread.local.data()->kcap());
	if (l4_msgtag_has_error(tag))
		error("cannot create more thread kernel-objects!");

	/* create initial gate for thread */
	_gate.local = thread_cap_factory().alloc(_thread.local);
}


void Platform_thread::_finalize_construction(const char *name)
{
	/* create irq for new thread */
	l4_msgtag_t tag = l4_factory_create_irq(L4_BASE_FACTORY_CAP,
	                                        _irq.local.data()->kcap());
	if (l4_msgtag_has_error(tag))
		PWRN("creating thread's irq failed");

	/* attach thread to irq */
	tag = l4_irq_attach(_irq.local.data()->kcap(), 0, _thread.local.data()->kcap());
	if (l4_msgtag_has_error(tag))
		PWRN("attaching thread's irq failed");

	/* set human readable name in kernel debugger */
	strncpy(_name, name, sizeof(_name));
	Fiasco::l4_debugger_set_object_name(_thread.local.data()->kcap(), name);

	l4_sched_param_t params;
	/* set priority of thread */
	if (_dl<=0)
		params = l4_sched_param_by_type(Fixed_prio, _prio, 0);
	else if(_prio<=0)
		params = l4_sched_param_by_type(Deadline, _dl, 0);
	else{
		PWRN("wrong scheduling type");
		return;
	}

	l4_scheduler_run_thread(L4_BASE_SCHEDULER_CAP, _thread.local.data()->kcap(),
	                        &params);
	_id = l4_utcb_mr()->mr[0];
	_arrival_time = l4_utcb_mr()->mr[1];
	//PDBG("sched cap %d\n", L4_BASE_SCHEDULER_CAP);
	//PDBG("name:%s id:%d arrived:%llu", name, _id, _arrival_time);
}


Weak_ptr<Address_space> Platform_thread::address_space()
{
	return _platform_pd->Address_space::weak_ptr();
}


unsigned long long Platform_thread::execution_time() const
{
	unsigned long long time = 0;

	if (_utcb) {
		l4_thread_stats_time(_thread.local.data()->kcap());
		time = *(l4_kernel_clock_t*)&l4_utcb_mr()->mr[0];
	}
	return time;
}

unsigned long long Platform_thread::start_time() const
{
	unsigned long long time = 0;
	if (_utcb) {
		l4_thread_stats_time(_thread.local.data()->kcap());
		time = l4_utcb_mr()->mr[2];
	}
	return time;
}

unsigned long long Platform_thread::arrival_time() const
{
	return _arrival_time;
}

unsigned Platform_thread::prio() const
{
	return _prio;
}

unsigned Platform_thread::id() const
{
	return _thread.local.data()->kcap();
}

long unsigned int Platform_thread::foc_id() const
{
	return _id;
}

unsigned long long Platform_thread::idle(unsigned num) const
{
	unsigned long long time = 0;

	l4_sched_cpu_set_t cpus = l4_sched_cpu_set(0, 0, num+1);

	if (_utcb) {
		l4_scheduler_idle_time(L4_BASE_SCHEDULER_CAP, &cpus);
		time = *(l4_kernel_clock_t*)&l4_utcb_mr()->mr[0];
	}

	return time;
}

bool Platform_thread::core_is_online(unsigned num) const
{
	return l4_scheduler_is_online(L4_BASE_SCHEDULER_CAP, num);
}

unsigned Platform_thread::num_cores() const
{
	l4_sched_cpu_set_t cpus = l4_sched_cpu_set(0, 0, 1);
	l4_umword_t cpus_max;
	return (unsigned)l4_scheduler_info(L4_BASE_SCHEDULER_CAP, &cpus_max, &cpus).raw;
}

void Platform_thread::rq(Genode::Dataspace_capability ds) const
{
	int *list = Genode::env()->rm_session()->attach(ds);
	l4_scheduler_get_rqs(L4_BASE_SCHEDULER_CAP);
	list[0]=(int)l4_utcb_mr()->mr[0];
	for(int i=1; i<=2*((int)l4_utcb_mr()->mr[0]);i++)
	{
		list[i]=l4_utcb_mr()->mr[i];
	}
}

void Platform_thread::dead(Genode::Dataspace_capability ds) const
{
	long long unsigned *list = Genode::env()->rm_session()->attach(ds);
	l4_scheduler_get_dead(L4_BASE_SCHEDULER_CAP);
	list[0]=(long long unsigned)l4_utcb_mr()->mr[0];
	for(long long unsigned i=1; i<=2*((long long unsigned)l4_utcb_mr()->mr[0]);i++)
	{
		list[i]=l4_utcb_mr()->mr[i];
	}
}

void Platform_thread::deploy_queue(Genode::Dataspace_capability ds) const
{
	int *list = Genode::env()->rm_session()->attach(ds);
	Fiasco::l4_msgtag_t tag = Fiasco::l4_scheduler_deploy_thread(Fiasco::L4_BASE_SCHEDULER_CAP, list);
	if (Fiasco::l4_error(tag)){
		PWRN("Scheduling queue has failed!\n");
		return;
	}
}

Platform_thread::Platform_thread(size_t, const char *name, unsigned prio,
                                 Affinity::Location location, addr_t)
: _state(DEAD),
  _core_thread(false),
  _thread(true),
  _irq(true),
  _utcb(0),
  _platform_pd(0),
  _pager_obj(0),
  _prio(Cpu_session::scale_priority(DEFAULT_PRIORITY, prio)),
  _dl(0)
{
	/* XXX remove const cast */
	((Core_cap_index *)_thread.local.data())->pt(this);
	_create_thread();
	_finalize_construction(name);
	affinity(location);
}


Platform_thread::Platform_thread(size_t, const char *name, unsigned prio, unsigned deadline,
                                 Affinity::Location location, addr_t)
: _state(DEAD),
  _core_thread(false),
  _thread(true),
  _irq(true),
  _utcb(0),
  _platform_pd(0),
  _pager_obj(0),
  _prio(Cpu_session::scale_priority(DEFAULT_PRIORITY, prio)),
  _dl(deadline)
{
	/* XXX remove const cast */
	((Core_cap_index *)_thread.local.data())->pt(this);
	_create_thread();
	_finalize_construction(name);
	affinity(location);
}


Platform_thread::Platform_thread(Core_cap_index* thread,
                                 Core_cap_index* irq, const char *name)
: _state(RUNNING),
  _core_thread(true),
  _thread(Native_capability(*thread), L4_BASE_THREAD_CAP),
  _irq(Native_capability(*irq)),
  _utcb(0),
  _platform_pd(0),
  _pager_obj(0),
  _prio(Cpu_session::scale_priority(DEFAULT_PRIORITY, 0)),
  _dl(0)
{
	/* XXX remove const cast */
	((Core_cap_index *)_thread.local.data())->pt(this);
	_finalize_construction(name);
}


Platform_thread::Platform_thread(const char *name)
: _state(DEAD),
  _core_thread(true),
  _thread(true),
  _irq(true),
  _utcb(0),
  _platform_pd(0),
  _pager_obj(0),
  _prio(Cpu_session::scale_priority(DEFAULT_PRIORITY, 0)),
  _dl(0)
{
	/* XXX remove const cast */
	((Core_cap_index *)_thread.local.data())->pt(this);
	_create_thread();
	_finalize_construction(name);
}


Platform_thread::~Platform_thread()
{
	thread_cap_factory().free(_gate.local);

	/*
	 * We inform our protection domain about thread destruction, which will end up in
	 * Thread::unbind()
	 */
	if (_platform_pd)
		_platform_pd->unbind_thread(this);
}
