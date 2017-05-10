/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.chronosync.scenarios.HubAndSpoke");

namespace ns3 {

std::unordered_map<std::string, std::pair<double, std::vector<double>>> delays;

static void DataEvent(std::string user_prefix, const std::string& content,
                      bool is_local) {
  NS_LOG_INFO("user_prefix=" << user_prefix << ", content={" << content
                             << "}, is_local="
                             << (is_local ? "true" : "false"));

  double now = Simulator::Now().GetSeconds();

  auto& entry = delays[content];
  if (is_local)
    entry.first = now;
  else
    entry.second.push_back(now);
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("100Mbps"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));
  Config::SetDefault("ns3::RateErrorModel::ErrorUnit",
                     StringValue("ERROR_UNIT_PACKET"));

  int N = 10;
  double TotalRunTimeSeconds = 3600.0;
  bool Synchronized = false;
  double LossRate = 0.0;
  std::string LinkDelay = "10ms";
  int LeavingNodes = 0;

  CommandLine cmd;
  cmd.AddValue("NumOfNodes", "Number of sync nodes in the group", N);
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds (> 20)",
               TotalRunTimeSeconds);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue("LinkDelay", "Delay of the underlying P2P channel", LinkDelay);
  cmd.AddValue("LeavingNodes",
               "Number of nodes randomly leaving the group after 20s",
               LeavingNodes);
  cmd.Parse(argc, argv);

  if (TotalRunTimeSeconds < 20.0) return -1;

  NodeContainer nodes;
  nodes.Create(N + 1);

  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue(LinkDelay));

  // Node 0 is central hub
  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  for (int i = 1; i <= N; ++i) {
    p2p.Install(nodes.Get(0), nodes.Get(i));
    nodes.Get(i)->GetDevice(0)->SetAttribute("ReceiveErrorModel",
                                             PointerValue(rem));
  }

  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1000);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll("/ndn",
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  Ptr<UniformRandomVariable> stop_time = CreateObject<UniformRandomVariable>();
  stop_time->SetAttribute("Min", DoubleValue(20.0));
  stop_time->SetAttribute("Max", DoubleValue(TotalRunTimeSeconds));

  for (int i = 1; i <= N; ++i) {
    ndn::AppHelper helper("ChronoSyncApp");
    helper.SetAttribute("SyncPrefix", StringValue("/ndn/broadcast/sync"));
    std::string user_prefix = "/Node" + std::to_string(i);
    helper.SetAttribute("UserPrefix", StringValue(user_prefix));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    if (i <= LeavingNodes)
      helper.SetAttribute("StopTime",
                          TimeValue(Seconds(stop_time->GetValue())));
    else
      helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    helper.Install(nodes.Get(i));

    ndn::FibHelper::AddRoute(nodes.Get(0), "/ndn/broadcast/sync", nodes.Get(i),
                             1);
    ndn::FibHelper::AddRoute(nodes.Get(0), user_prefix, nodes.Get(i), 1);

    ndn::FibHelper::AddRoute(nodes.Get(i), "/", nodes.Get(0), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), "/ndn/broadcast/sync", nodes.Get(0),
                             1);

    nodes.Get(i)->GetApplication(0)->TraceConnect("DataEvent", user_prefix,
                                                  MakeCallback(&DataEvent));
  }

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  Simulator::Run();
  Simulator::Destroy();

  std::string file_name = "results/D" + LinkDelay + "N" + std::to_string(N);
  if (Synchronized) file_name += "Sync";
  if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
  if (LeavingNodes > 0) file_name += "LN" + std::to_string(LeavingNodes);
  std::fstream fs(file_name, std::ios_base::out | std::ios_base::trunc);

  int count = 0;
  double average_delay = std::accumulate(
      delays.begin(), delays.end(), 0.0,
      [&count, &fs](double a, const decltype(delays)::value_type& b) {
        double gen_time = b.second.first;
        const auto& vec = b.second.second;
        count += vec.size();
        return a + std::accumulate(vec.begin(), vec.end(), 0.0,
                                   [gen_time, &fs](double c, double d) {
                                     fs << (d - gen_time) << std::endl;
                                     return c + d - gen_time;
                                   });
      });
  average_delay /= count;

  fs.close();

  std::cout << "Total number of data published is: " << delays.size()
            << std::endl;
  std::cout << "Total number of data propagated is: " << count << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }
