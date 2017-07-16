/*
 * \brief  Client-side cpu session interface
 * \author Christian Helmuth
 * \date   2006-07-12
 */

/*
 * Copyright (C) 2006-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__CPU_SESSION__CLIENT_H_
#define _INCLUDE__CPU_SESSION__CLIENT_H_

#include <cpu_session/capability.h>
#include <base/rpc_client.h>

namespace Genode { struct Cpu_session_client; }


struct Genode::Cpu_session_client : Rpc_client<Cpu_session>
{
	explicit Cpu_session_client(Cpu_session_capability session)
	: Rpc_client<Cpu_session>(session) { }

	Thread_capability
	create_thread(Capability<Pd_session> pd, Name const &name,
	              Affinity::Location affinity, Weight weight, addr_t utcb = 0) override {
		return call<Rpc_create_thread>(pd, name, affinity, weight, utcb); }

	Thread_capability
	create_fp_edf_thread(Capability<Pd_session> pd, Name const &name,
	              Affinity::Location affinity, Weight weight, addr_t utcb = 0,
	              unsigned priority = 0, unsigned deadline = 0) override {
		return call<Rpc_create_fp_edf_thread>(pd, name, affinity, weight, utcb,
	              	              	        priority, deadline); }

	int set_sched_type(unsigned core = 0, unsigned sched_type = 0){
		return call<Rpc_set_sched_type>(core, sched_type); }

	int get_sched_type(unsigned core = 0){
		return call<Rpc_get_sched_type>(core); }

	void set(Ram_session_capability ram_cap) override {
		call<Rpc_set>(ram_cap); }

	void kill_thread(Thread_capability thread) override {
		call<Rpc_kill_thread>(thread); }

	void exception_sigh(Signal_context_capability sigh) override {
		call<Rpc_exception_sigh>(sigh); }

	Affinity::Space affinity_space() const override {
		return call<Rpc_affinity_space>(); }

	Dataspace_capability trace_control() override {
		return call<Rpc_trace_control>(); }

	int ref_account(Cpu_session_capability session) override {
		return call<Rpc_ref_account>(session); }

	int transfer_quota(Cpu_session_capability session, size_t amount) override {
		return call<Rpc_transfer_quota>(session, amount); }

	void deploy_queue(Genode::Dataspace_capability ds) override {
		call<Rpc_deploy_queue>(ds); }

	void rq(Genode::Dataspace_capability ds) override {
		call<Rpc_rq>(ds); }

	void dead(Genode::Dataspace_capability ds) override {
		call<Rpc_dead>(ds); }

	Quota quota() override { return call<Rpc_quota>(); }

	Capability<Native_cpu> native_cpu() override { return call<Rpc_native_cpu>(); }
};

#endif /* _INCLUDE__CPU_SESSION__CLIENT_H_ */
