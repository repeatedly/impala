// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef SPARROW_SCHEDULER_H
#define SPARROW_SCHEDULER_H

#include <vector>
#include <string>

#include "common/status.h"
#include "gen-cpp/Types_types.h"  // for THostPort

namespace sparrow {

// Abstract scheduler and nameservice class.
// Given a list of resources and locations returns a list of hosts on which
// to execute plan fragments requiring those resources.
// At the moment, this simply returns a list of registered backends running
// on those hosts.
class Scheduler {
 public:
  virtual ~Scheduler() { }

  // List of server addresses. THostPort.ipdaddress is set, THostPort.hostname
  // may not be. See IMP-261 for plans to sort this out. 
  typedef std::vector<impala::THostPort> HostList;

  // Given a list of host / port pairs that represent data locations,
  // fills in hostports with host/port pairs of known ImpalaInternalServices
  // (that are running on those hosts or nearby).
  virtual impala::Status GetHosts(
      const HostList& data_locations, HostList* hostports) = 0;

  // Return a list of all hosts known to the scheduler
  virtual void GetAllKnownHosts(HostList* hostports) = 0;

  // Initialises the scheduler, acquiring all resources needed to make
  // scheduling decisions once this method returns.
  virtual impala::Status Init() = 0;

  // Scheduler should clean up all resources in Close()
  virtual void Close() = 0;
};

}

#endif
