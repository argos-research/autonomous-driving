/*
 * \brief  Registry containing possible sources of tracing data
 * \author Norman Feske
 * \date   2013-08-09
 */

/*
 * Copyright (C) 2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CORE__INCLUDE__TRACE__SOURCE_REGISTRY_H_
#define _CORE__INCLUDE__TRACE__SOURCE_REGISTRY_H_

#include <util/list.h>
#include <util/string.h>
#include <base/lock.h>
#include <base/trace/types.h>
#include <base/weak_ptr.h>

/* base-internal include */
#include <base/internal/trace_control.h>

namespace Genode { namespace Trace {
	class Source;
	class Source_owner;
	class Source_registry;

	/**
	 * Return singleton instance of trace-source registry
	 */
	Source_registry &sources();
} }


struct Genode::Trace::Source_owner { };


/**
 * Source of tracing data
 *
 * There is one instance per thread.
 */
class Genode::Trace::Source
:
	public Genode::Weak_object<Genode::Trace::Source>,
	public Genode::List<Genode::Trace::Source>::Element
{
	public:

		struct Info
		{
			Session_label      label;
			Thread_name        name;
			Execution_time     execution_time;
			Affinity::Location affinity;
			unsigned long long start_time;
			unsigned long long arrival_time;
			unsigned	   prio;
			unsigned	   id;
			unsigned	   foc_id;
			int		   pos_rq;
			Execution_time	   idle0;
			Execution_time	   idle1;
			Execution_time	   idle2;
			Execution_time	   idle3;
			bool		   core0_is_online;
			bool		   core1_is_online;
			bool		   core2_is_online;
			bool		   core3_is_online;
			unsigned	   num_cores;
			size_t 		   ram_quota;
			size_t 		   ram_used;
		};
		struct Dynamic_Info
		{
			Execution_time     execution_time;
			Affinity::Location affinity;
			unsigned long long start_time;
			unsigned long long arrival_time;
			unsigned	   prio;
			int		   pos_rq;
		};
		struct Static_Info
		{
			Session_label      label;
			Thread_name        name;
			unsigned	   id;
			unsigned	   foc_id;
		};
		struct Global_Info
		{
			Execution_time	   idle0;
			Execution_time	   idle1;
			Execution_time	   idle2;
			Execution_time	   idle3;
			bool		   core0_is_online;
			bool		   core1_is_online;
			bool		   core2_is_online;
			bool		   core3_is_online;
			unsigned	   num_cores;
		};

		/**
		 * Interface for querying trace-source information
		 */
		struct Info_accessor
		{
			virtual Info trace_source_info() const = 0;
			virtual Dynamic_Info dynamic_info() const = 0;
			virtual Static_Info static_info() const = 0;
			virtual Global_Info global_info() const = 0;
		};

	private:

		unsigned      const  _unique_id;
		Info_accessor const &_info;
		Control             &_control;
		Dataspace_capability _policy;
		Dataspace_capability _buffer;
		Source_owner  const *_owner = nullptr;
		unsigned	   id;
		unsigned	   foc_id;
		size_t _ram_quota=0;
		size_t _ram_used=0;

		static unsigned _alloc_unique_id();

	public:

		Source(Info_accessor const &info, Control &control)
		:
			_unique_id(_alloc_unique_id()), _info(info), _control(control)
		{ }

		~Source()
		{
			/* invalidate weak pointers to this object */
			lock_for_destruction();
		}

		void set_quota(size_t quota)
		{
			_ram_quota=quota;
		}

		void set_used(size_t quota)
		{
			_ram_used=quota;
		}


		/*************************************
		 ** Interface used by TRACE service **
		 *************************************/

		Info const info() const 
		{
			
			Info info;
			Dynamic_Info dyn=_info.dynamic_info();
			Static_Info stat=_info.static_info();
			info.label=stat.label;
			info.name=stat.name;
			info.id=stat.id;
			info.foc_id=stat.foc_id;
			info.execution_time=dyn.execution_time;
			info.start_time=dyn.start_time;
			info.arrival_time=dyn.arrival_time;
			info.affinity=dyn.affinity;
			info.prio=dyn.prio;
			info.ram_quota=_ram_quota;
			info.ram_used=_ram_used;
			info.pos_rq=dyn.pos_rq;
			return info;
		}
		Info const sched_info() const
		{
			Info info;
			Global_Info global=_info.global_info();
			info.idle0=global.idle0;
			info.idle1=global.idle1;
			info.idle2=global.idle2;
			info.idle3=global.idle3;
			info.core0_is_online=global.core0_is_online;
			info.core1_is_online=global.core1_is_online;
			info.core2_is_online=global.core2_is_online;
			info.core3_is_online=global.core3_is_online;
			info.num_cores=global.num_cores;
			return info;
		}

		void trace(Dataspace_capability policy, Dataspace_capability buffer)
		{
			_buffer = buffer;
			_policy = policy;
			_control.trace();
		}

		void enable()  { _control.enable(); }
		void disable() { _control.disable(); }

		bool try_acquire(Source_owner const *new_owner)
		{
			if (_owner && _owner != new_owner)
				return false;

			_owner = new_owner;
			return true;
		}

		bool owned_by(Source_owner const *owner) { return owner == _owner; }

		void release_ownership(Source_owner const *owner)
		{
			if (owned_by(owner))
				_owner = 0;
		}

		bool error()   const { return _control.has_error(); }
		bool enabled() const { return _control.enabled(); }


		/***********************************
		 ** Interface used by CPU service **
		 ***********************************/

		Dataspace_capability buffer()    const { return _buffer; }
		Dataspace_capability policy()    const { return _policy; }
		unsigned             unique_id() const { return _unique_id; }
};


/**
 * Registry to tracing sources
 *
 * There is a single instance within core.
 */
class Genode::Trace::Source_registry
{
	private:

		Lock         _lock;
		List<Source> _entries;

	public:

		/***********************************
		 ** Interface used by CPU service **
		 ***********************************/

		void insert(Source *entry)
		{
			Lock::Guard guard(_lock);

			_entries.insert(entry);
		}

		void remove(Source *entry)
		{
			Lock::Guard guard(_lock);
			_entries.remove(entry);
		}
		
		void set_quota(size_t _ram_quota, String<160> _label)
		{
			Lock::Guard guard(_lock);
			if(strcmp(_label.string(), "")==0) { _label="core";}
			Source *entry=_entries.first();
			Trace::Source::Info info=entry->info();
			if(strcmp(info.label.string(), _label.string())==0) {
				entry->set_quota(_ram_quota);	
			}
			while(entry->next()!=NULL) {
				entry=entry->next();
				info=entry->info();
				if(strcmp(info.label.string(), _label.string())==0) {
					entry->set_quota(_ram_quota);
				}	
			}
			if(strcmp(info.label.string(), _label.string())==0) {
				entry->set_quota(_ram_quota);	
			}		
		}

		void set_used(size_t _ram_used, String<160> _label)
		{
			Lock::Guard guard(_lock);
			if(strcmp(_label.string(), "")==0) { _label="core";}
			Source *entry=_entries.first();
			Trace::Source::Info info=entry->info();
			if(strcmp(info.label.string(), _label.string())==0) {
				entry->set_used(_ram_used);	
			}
			while(entry->next()!=NULL) {
				entry=entry->next();
				info=entry->info();
				if(strcmp(info.label.string(), _label.string())==0) {
					entry->set_used(_ram_used);
				}	
			}
			if(strcmp(info.label.string(), _label.string())==0) {
				entry->set_used(_ram_used);	
			}
			
			
		}


		/*************************************
		 ** Interface used by TRACE service **
		 *************************************/

		template <typename TEST, typename INSERT>
		void export_sources(TEST &test, INSERT &insert)
		{
			for (Source *s = _entries.first(); s; s = s->next())
				if (!test(s->unique_id())) {
					Source::Info const info = s->info();
					insert(s->unique_id(), s->weak_ptr(), info.label, info.name);
				}
		}

};

#endif /* _CORE__INCLUDE__TRACE__SOURCE_REGISTRY_H_ */
