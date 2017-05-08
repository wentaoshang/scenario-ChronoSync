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

NS_LOG_COMPONENT_DEFINE("ns3.ndn.chronosync.scenarios.Campus");

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
  double TotalRunTimeSeconds = 3600.0;
  double LossRate = 0.0;
  bool Synchronized = false;

  CommandLine cmd;
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("topologies/campus.txt");
  topologyReader.Read();

  // Install Ndn stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1000);
  ndnHelper.InstallAll();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll("/ndn/broadcast/sync",
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));

  for (int i = 1; i <= 10; ++i) {
    std::string nid = 'n' + std::to_string(i);
    Ptr<Node> node = Names::Find<Node>(nid);

    ndn::AppHelper helper("ChronoSyncApp");
    helper.SetAttribute("SyncPrefix", StringValue("/ndn/broadcast/sync"));
    std::string user_prefix = '/' + nid;
    helper.SetAttribute("UserPrefix", StringValue(user_prefix));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.Install(node);

    ndnGlobalRoutingHelper.AddOrigins(user_prefix, node);
    ndnGlobalRoutingHelper.AddOrigins("/ndn/broadcast/sync", node);

    node->GetApplication(0)->TraceConnect("DataEvent", nid,
                                          MakeCallback(&DataEvent));
    node->GetDevice(0)->SetAttribute("ReceiveErrorModel", PointerValue(rem));
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  Simulator::Run();
  Simulator::Destroy();

  std::string file_name =
      "results/CS-CampusRunTime" + std::to_string(TotalRunTimeSeconds);
  if (Synchronized) file_name += "Sync";
  if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
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
