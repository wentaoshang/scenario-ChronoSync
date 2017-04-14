/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors
 * and contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_APP_HPP_
#define CHRONOSYNC_APP_HPP_

#include "ns3/ndnSIM-module.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"

#include "chronosync-node.hpp"

namespace ns3 {
namespace ndn {

class ChronoSyncApp : public Application {
 public:
  typedef void (*DataEventTraceCallback)(const std::string&, bool);

  static TypeId GetTypeId() {
    static TypeId tid =
        TypeId("ChronoSyncApp")
            .SetParent<Application>()
            .AddConstructor<ChronoSyncApp>()
            .AddAttribute("SyncPrefix", "Sync Prefix", StringValue("/"),
                          MakeNameAccessor(&ChronoSyncApp::sync_prefix_),
                          MakeNameChecker())
            .AddAttribute("UserPrefix", "User Prefix", StringValue("/"),
                          MakeNameAccessor(&ChronoSyncApp::user_prefix_),
                          MakeNameChecker())
            .AddAttribute("RoutingPrefix", "Routing Prefix", StringValue("/"),
                          MakeNameAccessor(&ChronoSyncApp::routing_prefix_),
                          MakeNameChecker())
            .AddAttribute(
                "RandomSeed", "Seed used for the random number generator.",
                UintegerValue(0), MakeUintegerAccessor(&ChronoSyncApp::seed_),
                MakeUintegerChecker<uint32_t>())
            .AddTraceSource(
                "DataEvent",
                "Event of publishing or receiving new data in the sync node.",
                MakeTraceSourceAccessor(&ChronoSyncApp::data_event_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::DataEventTraceCallback");

    return tid;
  }

 protected:
  // inherited from Application base class.
  virtual void StartApplication() {
    instance_.reset(new ::ndn::ChronoSyncNode(seed_, sync_prefix_, user_prefix_,
                                              routing_prefix_,
                                              ndn::StackHelper::getKeyChain()));
    instance_->Init();
    instance_->Run();
  }

  virtual void StopApplication() { instance_.reset(); }

 private:
  std::unique_ptr<::ndn::ChronoSyncNode> instance_;
  Name sync_prefix_;
  Name user_prefix_;
  Name routing_prefix_;
  uint32_t seed_;
  TracedCallback<const std::string&, bool> data_event_trace_;
};

}  // namespace ndn
}  // namespace ns3

#endif  // CHRONOSYNC_APP_HPP_
