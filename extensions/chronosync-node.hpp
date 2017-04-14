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

#ifndef CHRONOSYNC_NODE_HPP_
#define CHRONOSYNC_NODE_HPP_

#include <functional>
#include <random>

#include "src/socket.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn {

class ChronoSyncNode {
 public:
  using DataEventTraceCb = std::function<void(const std::string&, bool)>;

  ChronoSyncNode(uint32_t seed, const Name& sync_prefix,
                 const Name& user_prefix, const Name& routing_prefix,
                 KeyChain& keychain);

  void PublishData();

  void ProcessData(const shared_ptr<const Data>& data);

  void ProcessSyncUpdate(
      const std::vector<chronosync::MissingDataInfo>& updates);

  void Init();

  void Run();

  void ConnectDataEventTrace(DataEventTraceCb cb) {
    data_event_trace_.connect(cb);
  }

 private:
  boost::asio::io_service io_service_;
  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;

  Name sync_prefix_;
  Name user_prefix_;
  Name routing_prefix_;

  std::unique_ptr<chronosync::Socket> socket_;

  uint32_t seed_;
  std::mt19937 rengine_;
  std::uniform_int_distribution<> rdist_;

  uint64_t counter_ = 0;
  util::Signal<ChronoSyncNode, const std::string&, bool> data_event_trace_;
};

}  // namespace ndn

#endif  // CHRONOSYNC_NODE_HPP_
