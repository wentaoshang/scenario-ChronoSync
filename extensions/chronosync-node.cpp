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

#include "chronosync-node.hpp"

namespace ndn {

ChronoSyncNode::ChronoSyncNode(uint32_t seed, const Name& sync_prefix,
                               const Name& user_prefix,
                               const Name& routing_prefix, KeyChain& keychain,
                               double data_rate)
    : face_(io_service_),
      scheduler_(io_service_),
      key_chain_(keychain),
      sync_prefix_(sync_prefix),
      user_prefix_(user_prefix),
      routing_prefix_(routing_prefix),
      seed_(seed),
      rengine_(seed),
      rdist_(data_rate) {}

void ChronoSyncNode::PublishData() {
  std::string msg = user_prefix_.toUri() + ":" + std::to_string(++counter_);
  socket_->publishData(reinterpret_cast<const uint8_t*>(msg.data()), msg.size(),
                       ndn::time::milliseconds(4000));
  data_event_trace_(msg, true);

  scheduler_.scheduleEvent(
      ndn::time::milliseconds(static_cast<int>(1000.0 * rdist_(rengine_))),
      std::bind(&ChronoSyncNode::PublishData, this));
}

void ChronoSyncNode::ProcessData(const shared_ptr<const Data>& data) {
  std::string msg(reinterpret_cast<const char*>(data->getContent().value()),
                  data->getContent().value_size());
  data_event_trace_(msg, false);
}

void ChronoSyncNode::ProcessSyncUpdate(
    const std::vector<chronosync::MissingDataInfo>& updates) {
  if (updates.empty()) {
    return;
  }

  for (size_t i = 0; i < updates.size(); ++i) {
    for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high;
         ++seq) {
      socket_->fetchData(updates[i].session, seq,
                         std::bind(&ChronoSyncNode::ProcessData, this, _1), 2);
    }
  }
}

void ChronoSyncNode::Init() {
  Name routable_user_prefix;
  routable_user_prefix.append(routing_prefix_).append(user_prefix_);

  socket_.reset(new chronosync::Socket(
      sync_prefix_, routable_user_prefix, face_, key_chain_, seed_,
      std::bind(&ChronoSyncNode::ProcessSyncUpdate, this, _1)));
}

void ChronoSyncNode::Run() {
  scheduler_.scheduleEvent(
      ndn::time::milliseconds(static_cast<int>(1000.0 * rdist_(rengine_))),
      std::bind(&ChronoSyncNode::PublishData, this));
}

}  // namespace ndn
