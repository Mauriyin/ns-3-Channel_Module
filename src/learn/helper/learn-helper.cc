/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/learn.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "learn-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LearnHelper");

LearnHelper::LearnHelper() : top(0)
{
	m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
	m_deviceFactory.SetTypeId("ns3::LearnNetDevice");
	m_channelFactory.SetTypeId("ns3::LearnChannel");
}

void LearnHelper::SetQueue(std::string type,
						   std::string n1, const AttributeValue &v1,
						   std::string n2, const AttributeValue &v2,
						   std::string n3, const AttributeValue &v3,
						   std::string n4, const AttributeValue &v4)
{
	QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

	m_queueFactory.SetTypeId(type);
	m_queueFactory.Set(n1, v1);
	m_queueFactory.Set(n2, v2);
	m_queueFactory.Set(n3, v3);
	m_queueFactory.Set(n4, v4);
}

void LearnHelper::SetDeviceAttribute(std::string n1, const AttributeValue &v1)
{
	m_deviceFactory.Set(n1, v1);
}

void LearnHelper::SetChannelAttribute(std::string n1, const AttributeValue &v1)
{
	m_channelFactory.Set(n1, v1);
}

void LearnHelper::EnablePcapInternal(std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
	//
	// All of the Pcap enable functions vector through here including the ones
	// that are wandering through all of devices on perhaps all of the nodes in
	// the system.  We can only deal with devices of type LearnNetDevice.
	//
	Ptr<LearnNetDevice> device = nd->GetObject<LearnNetDevice>();
	if (device == 0)
	{
		NS_LOG_INFO("LearnHelper::EnablePcapInternal(): Device " << device << " not of type ns3::LearnNetDevice");
		return;
	}

	PcapHelper pcapHelper;

	std::string filename;
	if (explicitFilename)
	{
		filename = prefix;
	}
	else
	{
		filename = pcapHelper.GetFilenameFromDevice(prefix, device);
	}

	Ptr<PcapFileWrapper> file = pcapHelper.CreateFile(filename, std::ios::out,
													  PcapHelper::DLT_PPP);
	pcapHelper.HookDefaultSink<LearnNetDevice>(device, "PromiscSniffer", file);
}

void LearnHelper::EnableAsciiInternal(
	Ptr<OutputStreamWrapper> stream,
	std::string prefix,
	Ptr<NetDevice> nd,
	bool explicitFilename)
{
	//
	// All of the ascii enable functions vector through here including the ones
	// that are wandering through all of devices on perhaps all of the nodes in
	// the system.  We can only deal with devices of type LearnNetDevice.
	//
	Ptr<LearnNetDevice> device = nd->GetObject<LearnNetDevice>();
	if (device == 0)
	{
		NS_LOG_INFO("LearnHelper::EnableAsciiInternal(): Device " << device << " not of type ns3::LearnNetDevice");
		return;
	}

	//
	// Our default trace sinks are going to use packet printing, so we have to
	// make sure that is turned on.
	//
	Packet::EnablePrinting();

	//
	// If we are not provided an OutputStreamWrapper, we are expected to create
	// one using the usual trace filename conventions and do a Hook*WithoutContext
	// since there will be one file per context and therefore the context would
	// be redundant.
	//
	if (stream == 0)
	{
		//
		// Set up an output stream object to deal with private ofstream copy
		// constructor and lifetime issues.  Let the helper decide the actual
		// name of the file given the prefix.
		//
		AsciiTraceHelper asciiTraceHelper;

		std::string filename;
		if (explicitFilename)
		{
			filename = prefix;
		}
		else
		{
			filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
		}

		Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream(filename);

		//
		// The MacRx trace source provides our "r" event.
		//
		asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<LearnNetDevice>(device, "MacRx", theStream);

		//
		// The "+", '-', and 'd' events are driven by trace sources actually in the
		// transmit queue.
		//
		Ptr<Queue<Packet>> queue = device->GetQueue();
		asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(queue, "Enqueue", theStream);
		asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(queue, "Drop", theStream);
		asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(queue, "Dequeue", theStream);

		// PhyRxDrop trace source for "d" event
		asciiTraceHelper.HookDefaultDropSinkWithoutContext<LearnNetDevice>(device, "PhyRxDrop", theStream);

		return;
	}

	//
	// If we are provided an OutputStreamWrapper, we are expected to use it, and
	// to providd a context.  We are free to come up with our own context if we
	// want, and use the AsciiTraceHelper Hook*WithContext functions, but for
	// compatibility and simplicity, we just use Config::Connect and let it deal
	// with the context.
	//
	// Note that we are going to use the default trace sinks provided by the
	// ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
	// but the default trace sinks are actually publicly available static
	// functions that are always there waiting for just such a case.
	//
	uint32_t nodeid = nd->GetNode()->GetId();
	uint32_t deviceid = nd->GetIfIndex();
	std::ostringstream oss;

	oss << "/NodeList/" << nd->GetNode()->GetId() << "/DeviceList/" << deviceid << "/$ns3::LearnNetDevice/MacRx";
	Config::Connect(oss.str(), MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

	oss.str("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::LearnNetDevice/TxQueue/Enqueue";
	Config::Connect(oss.str(), MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

	oss.str("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::LearnNetDevice/TxQueue/Dequeue";
	Config::Connect(oss.str(), MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

	oss.str("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::LearnNetDevice/TxQueue/Drop";
	Config::Connect(oss.str(), MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

	oss.str("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::LearnNetDevice/PhyRxDrop";
	Config::Connect(oss.str(), MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer
LearnHelper::Install(NodeContainer c)
{
	NetDeviceContainer container;
	Ptr<LearnChannel> channel = m_channelFactory.Create<LearnChannel>();
	for (decltype(c.GetN()) i = 0; i < c.GetN(); ++i)
	{
		Ptr<Node> n = c.Get(i);
		Ptr<LearnNetDevice> dev = m_deviceFactory.Create<LearnNetDevice>();
		dev->SetAddress(Mac48Address::Allocate());
		dev->SetXY(m_xs[i], m_ys[i]);
		n->AddDevice(dev);
		Ptr<Queue<Packet>> queue = m_queueFactory.Create<Queue<Packet>>();
		dev->SetQueue(queue);

		Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface>();
		ndqi->GetTxQueue(0)->ConnectQueueTraces(queue);
		dev->AggregateObject(ndqi);
		dev->Attach(channel);
		container.Add(dev);
	}
	return container;
}

} // namespace ns3
