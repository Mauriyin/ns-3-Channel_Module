/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/learn-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestLearn");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("Learn", LOG_INFO);
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  LearnHelper learn;
  learn.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  learn.SetChannelAttribute ("DelayFac", StringValue ("2ms"));
  learn.AddPosition (0, 0);
  learn.AddPosition (1, 0);
  learn.AddPosition (0, 1);
  learn.AddPosition (1, 1);

  NetDeviceContainer devices;
  devices = learn.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (1));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (1));
  clientApps.Start (Seconds (0));
  clientApps.Stop (Seconds (1));

  // UdpEchoClientHelper echoClient2 (interfaces.GetAddress (0), 9);
  // echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
  // echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  // ApplicationContainer clientApps2 = echoClient2.Install (nodes.Get (2));
  // clientApps2.Start (Seconds (0));
  // clientApps2.Stop (Seconds (0.001));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
