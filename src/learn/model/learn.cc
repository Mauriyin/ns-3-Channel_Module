/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "learn.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("Learn");

/////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(LearnChannel);

TypeId
LearnChannel::GetTypeId(void)
{
	static TypeId tid =
		TypeId("ns3::LearnChannel")
			.SetParent<Channel>()
			.SetGroupName("Learn")
			.AddConstructor<LearnChannel>()
			.AddAttribute("DelayFac", "Propagation delay through the channel",
						  TimeValue(Seconds(0)), MakeTimeAccessor(&LearnChannel::m_delay_fac),
						  MakeTimeChecker());
	return tid;
}

//
// By default, you get a channel that
// has an "infitely" fast transmission speed and zero delay.
LearnChannel::LearnChannel() : Channel(), m_delay_fac(Seconds(0.)), m_nDevices(0)
{
	NS_LOG_FUNCTION_NOARGS();
	memset(m_dist, 0, sizeof(m_dist));
}

void LearnChannel::Attach(Ptr<LearnNetDevice> device)
{
	NS_LOG_FUNCTION(this << device);
	NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Too much devices");
	NS_ASSERT(device != 0);

	m_devices[m_nDevices] = device;

	for (decltype(m_nDevices) i = 0; i < m_nDevices; ++i)
	{
		m_dist[m_nDevices][i] = m_dist[i][m_nDevices] = GetDist(m_devices[i], m_devices[m_nDevices]);
	}
	++m_nDevices;
}

bool LearnChannel::TransmitStart(Ptr<const Packet> p, Ptr<LearnNetDevice> src, Time txTime)
{
	NS_LOG_FUNCTION(this << p << src);
	NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
	for (decltype(m_nDevices) i = 0; i < m_nDevices; ++i)
	{
		if (m_devices[i] == src)
		{
			continue;
		}
		Simulator::ScheduleWithContext(
			m_devices[i]->GetNode()->GetId(), txTime + GetDelay(src, m_devices[i]),
			&LearnNetDevice::Receive, m_devices[i], p->Copy(), m_devices[i]);
	}

	return true;
}

std::size_t
LearnChannel::GetNDevices(void) const
{
	NS_LOG_FUNCTION_NOARGS();
	return m_nDevices;
}

Ptr<LearnNetDevice>
LearnChannel::GetLearnDevice(std::size_t i) const
{
	NS_LOG_FUNCTION_NOARGS();
	NS_ASSERT(i < N_DEVICES);
	return m_devices[i];
}

Ptr<NetDevice>
LearnChannel::GetDevice(std::size_t i) const
{
	NS_LOG_FUNCTION_NOARGS();
	return GetLearnDevice(i);
}

Time LearnChannel::GetDelay(Ptr<LearnNetDevice> n1, Ptr<LearnNetDevice> n2) const
{
	return m_delay_fac * GetDist(n1, n2);
}

Time LearnChannel::GetDelayFac(void) const
{
	return m_delay_fac;
}

double
LearnChannel::GetDist(Ptr<LearnNetDevice> n1, Ptr<LearnNetDevice> n2) const
{
	double x1 = n1->GetX();
	double x2 = n2->GetX();
	double y1 = n1->GetY();
	double y2 = n2->GetY();
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(LearnNetDevice);

TypeId
LearnNetDevice::GetTypeId(void)
{
	static TypeId tid =
		TypeId("ns3::LearnNetDevice")
			.SetParent<NetDevice>()
			.SetGroupName("Learn")
			.AddConstructor<LearnNetDevice>()
			.AddAttribute("Mtu", "The MAC-level Maximum Transmission Unit",
						  UintegerValue(DEFAULT_MTU),
						  MakeUintegerAccessor(&LearnNetDevice::SetMtu, &LearnNetDevice::GetMtu),
						  MakeUintegerChecker<uint16_t>())
			.AddAttribute("Address", "The MAC address of this device.",
						  Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
						  MakeMac48AddressAccessor(&LearnNetDevice::m_address),
						  MakeMac48AddressChecker())
			.AddAttribute("DataRate", "The default data rate for point to point links",
						  DataRateValue(DataRate("32768b/s")),
						  MakeDataRateAccessor(&LearnNetDevice::m_bps), MakeDataRateChecker())
			.AddAttribute("ReceiveErrorModel",
						  "The receiver error model used to simulate packet loss", PointerValue(),
						  MakePointerAccessor(&LearnNetDevice::m_receiveErrorModel),
						  MakePointerChecker<ErrorModel>())
			.AddAttribute("InterframeGap", "The time to wait between packet (frame) transmissions",
						  TimeValue(Seconds(0.0)),
						  MakeTimeAccessor(&LearnNetDevice::m_tInterframeGap), MakeTimeChecker())

			//
			// Transmit queueing discipline for the device which includes its own set
			// of trace hooks.
			//
			.AddAttribute("TxQueue", "A queue to use as the transmit queue in the device.",
						  PointerValue(), MakePointerAccessor(&LearnNetDevice::m_queue),
						  MakePointerChecker<Queue<Packet>>())

			//
			// Trace sources at the "top" of the net device, where packets transition
			// to/from higher layers.
			//
			.AddTraceSource("MacTx",
							"Trace source indicating a packet has arrived "
							"for transmission by this device",
							MakeTraceSourceAccessor(&LearnNetDevice::m_macTxTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("MacTxDrop",
							"Trace source indicating a packet has been dropped "
							"by the device before transmission",
							MakeTraceSourceAccessor(&LearnNetDevice::m_macTxDropTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("MacPromiscRx",
							"A packet has been received by this device, "
							"has been passed up from the physical layer "
							"and is being forwarded up the local protocol stack.  "
							"This is a promiscuous trace,",
							MakeTraceSourceAccessor(&LearnNetDevice::m_macPromiscRxTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("MacRx",
							"A packet has been received by this device, "
							"has been passed up from the physical layer "
							"and is being forwarded up the local protocol stack.  "
							"This is a non-promiscuous trace,",
							MakeTraceSourceAccessor(&LearnNetDevice::m_macRxTrace),
							"ns3::Packet::TracedCallback")

			//
			// Trace sources at the "bottom" of the net device, where packets transition
			// to/from the channel.
			//
			.AddTraceSource("PhyTxBegin",
							"Trace source indicating a packet has begun "
							"transmitting over the channel",
							MakeTraceSourceAccessor(&LearnNetDevice::m_phyTxBeginTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("PhyTxEnd",
							"Trace source indicating a packet has been "
							"completely transmitted over the channel",
							MakeTraceSourceAccessor(&LearnNetDevice::m_phyTxEndTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("PhyTxDrop",
							"Trace source indicating a packet has been "
							"dropped by the device during transmission",
							MakeTraceSourceAccessor(&LearnNetDevice::m_phyTxDropTrace),
							"ns3::Packet::TracedCallback")

			.AddTraceSource("PhyRxEnd",
							"Trace source indicating a packet has been "
							"completely received by the device",
							MakeTraceSourceAccessor(&LearnNetDevice::m_phyRxEndTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("PhyRxDrop",
							"Trace source indicating a packet has been "
							"dropped by the device during reception",
							MakeTraceSourceAccessor(&LearnNetDevice::m_phyRxDropTrace),
							"ns3::Packet::TracedCallback")

			//
			// Trace sources designed to simulate a packet sniffer facility (tcpdump).
			// Note that there is really no difference between promiscuous and
			// non-promiscuous traces in a point-to-point link.
			//
			.AddTraceSource("Sniffer",
							"Trace source simulating a non-promiscuous packet sniffer "
							"attached to the device",
							MakeTraceSourceAccessor(&LearnNetDevice::m_snifferTrace),
							"ns3::Packet::TracedCallback")
			.AddTraceSource("PromiscSniffer",
							"Trace source simulating a promiscuous packet sniffer "
							"attached to the device",
							MakeTraceSourceAccessor(&LearnNetDevice::m_promiscSnifferTrace),
							"ns3::Packet::TracedCallback");
	return tid;
}

LearnNetDevice::LearnNetDevice()
	: m_txMachineState(READY), m_channel(0), m_linkUp(false), m_currentPkt(0)
{
	NS_LOG_FUNCTION(this);
}

LearnNetDevice::~LearnNetDevice()
{
	NS_LOG_FUNCTION(this);
}

void LearnNetDevice::DoDispose()
{
	NS_LOG_FUNCTION(this);
	m_node = 0;
	m_channel = 0;
	m_receiveErrorModel = 0;
	m_currentPkt = 0;
	m_queue = 0;
	NetDevice::DoDispose();
}

void LearnNetDevice::SetDataRate(DataRate bps)
{
	NS_LOG_FUNCTION(this);
	m_bps = bps;
}

void LearnNetDevice::SetInterframeGap(Time t)
{
	NS_LOG_FUNCTION(this << t.GetSeconds());
	m_tInterframeGap = t;
}

bool LearnNetDevice::TransmitStart(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

	//
	// This function is called to start the process of transmitting a packet.
	// We need to tell the channel that we've started wiggling the wire and
	// schedule an event that will be executed when the transmission is complete.
	//
	NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
	m_txMachineState = BUSY;
	m_currentPkt = p;
	m_phyTxBeginTrace(m_currentPkt);

	Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
	Time txCompleteTime = txTime + m_tInterframeGap;

	NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
	Simulator::Schedule(txCompleteTime, &LearnNetDevice::TransmitComplete, this);

	bool result = m_channel->TransmitStart(p, this, txTime);
	if (result == false)
	{
		m_phyTxDropTrace(p);
	}
	return result;
}

void LearnNetDevice::TransmitComplete(void)
{
	NS_LOG_FUNCTION(this);

	//
	// This function is called to when we're all done transmitting a packet.
	// We try and pull another packet off of the transmit queue.  If the queue
	// is empty, we are done, otherwise we need to start transmitting the
	// next packet.
	//
	NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
	m_txMachineState = READY;

	NS_ASSERT_MSG(m_currentPkt != 0, "LearnNetDevice::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace(m_currentPkt);
	m_currentPkt = 0;

	Ptr<Packet> p = m_queue->Dequeue();
	if (p == 0)
	{
		NS_LOG_LOGIC("No pending packets in device queue after tx complete");
		return;
	}

	//
	// Got another packet off of the queue, so start the transmit process again.
	//
	m_snifferTrace(p);
	m_promiscSnifferTrace(p);
	TransmitStart(p);
}

bool LearnNetDevice::Attach(Ptr<LearnChannel> ch)
{
	NS_LOG_FUNCTION(this << &ch);

	m_channel = ch;

	m_channel->Attach(this);

	//
	// This device is up whenever it is attached to a channel.  A better plan
	// would be to have the link come up when both devices are attached, but this
	// is not done for now.
	//
	NotifyLinkUp();
	return true;
}

void LearnNetDevice::SetQueue(Ptr<Queue<Packet>> q)
{
	NS_LOG_FUNCTION(this << q);
	m_queue = q;
}

void LearnNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em)
{
	NS_LOG_FUNCTION(this << em);
	m_receiveErrorModel = em;
}

void LearnNetDevice::Receive(Ptr<Packet> packet, Ptr<LearnNetDevice> src)
{
	NS_LOG_FUNCTION(this << packet);
	uint16_t protocol = 0x0800;

	if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
	{
		//
		// If we have an error model and it indicates that it is time to lose a
		// corrupted packet, don't forward this packet up, let it go.
		//
		m_phyRxDropTrace(packet);
	}
	else
	{
		//
		// Hit the trace hooks.  All of these hooks are in the same place in this
		// device because it is so simple, but this is not usually the case in
		// more complicated devices.
		//
		m_snifferTrace(packet);
		m_promiscSnifferTrace(packet);
		m_phyRxEndTrace(packet);

		//
		// Trace sinks will expect complete packets, not packets without some of the
		// headers.
		//
		Ptr<Packet> originalPacket = packet->Copy();

		//
		// Strip off the point-to-point protocol header and forward this packet
		// up the protocol stack.  Since this is a simple point-to-point link,
		// there is no difference in what the promisc callback sees and what the
		// normal receive callback sees.
		//

		if (!m_promiscCallback.IsNull())
		{
			NS_LOG_LOGIC("call m_promiscCallback");
			m_macPromiscRxTrace(originalPacket);
			m_promiscCallback(this, packet, protocol, src->GetAddress(), GetAddress(),
							  NetDevice::PACKET_HOST);
		}
		NS_LOG_UNCOND("call m_rxCallback");
		m_macRxTrace(originalPacket);
		m_rxCallback(this, packet, protocol, src->GetAddress());
	}
}

Ptr<Queue<Packet>>
LearnNetDevice::GetQueue(void) const
{
	NS_LOG_FUNCTION(this);
	return m_queue;
}

void LearnNetDevice::NotifyLinkUp(void)
{
	NS_LOG_FUNCTION(this);
	m_linkUp = true;
	m_linkChangeCallbacks();
}

void LearnNetDevice::SetIfIndex(const uint32_t index)
{
	NS_LOG_FUNCTION(this);
	m_ifIndex = index;
}

uint32_t
LearnNetDevice::GetIfIndex(void) const
{
	return m_ifIndex;
}

Ptr<Channel>
LearnNetDevice::GetChannel(void) const
{
	return m_channel;
}

void LearnNetDevice::SetAddress(Address address)
{
	NS_LOG_FUNCTION(this << address);
	m_address = Mac48Address::ConvertFrom(address);
}

Address
LearnNetDevice::GetAddress(void) const
{
	return m_address;
}

bool LearnNetDevice::IsLinkUp(void) const
{
	NS_LOG_FUNCTION(this);
	return m_linkUp;
}

void LearnNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
	NS_LOG_FUNCTION(this);
	m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool LearnNetDevice::IsBroadcast(void) const
{
	NS_LOG_FUNCTION(this);
	return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
LearnNetDevice::GetBroadcast(void) const
{
	NS_LOG_FUNCTION(this);
	return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool LearnNetDevice::IsMulticast(void) const
{
	NS_LOG_FUNCTION(this);
	return false;
}

Address
LearnNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
	NS_LOG_FUNCTION(this);
	return Address();
}

Address
LearnNetDevice::GetMulticast(Ipv6Address addr) const
{
	NS_LOG_FUNCTION(this << addr);
	return Address();
}

bool LearnNetDevice::IsPointToPoint(void) const
{
	NS_LOG_FUNCTION(this);
	return false;
}

bool LearnNetDevice::IsBridge(void) const
{
	NS_LOG_FUNCTION(this);
	return false;
}

bool LearnNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	if (IsLinkUp() == false)
	{
		m_macTxDropTrace(packet);
		return false;
	}
	m_macTxTrace(packet);

	if (m_queue->Enqueue(packet))
	{
		if (m_txMachineState == READY)
		{
			NS_LOG_UNCOND("Net Device Send");
			packet = m_queue->Dequeue();
			m_snifferTrace(packet);
			m_promiscSnifferTrace(packet);
			bool ret = TransmitStart(packet);
			return ret;
		}
		return true;
	}

	m_macTxDropTrace(packet);
	return false;
}

bool LearnNetDevice::SendFrom(Ptr<Packet> packet, const Address &source, const Address &dest,
							  uint16_t protocolNumber)
{
	NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);
	return false;
}

Ptr<Node>
LearnNetDevice::GetNode(void) const
{
	return m_node;
}

void LearnNetDevice::SetNode(Ptr<Node> node)
{
	NS_LOG_FUNCTION(this);
	m_node = node;
}

bool LearnNetDevice::NeedsArp(void) const
{
	NS_LOG_FUNCTION(this);
	return false;
}

void LearnNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
	NS_LOG_FUNCTION(this);
	m_rxCallback = cb;
}

void LearnNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
	m_promiscCallback = cb;
}

bool LearnNetDevice::SupportsSendFrom(void) const
{
	NS_LOG_FUNCTION(this);
	return false;
}

bool LearnNetDevice::SetMtu(uint16_t mtu)
{
	NS_LOG_FUNCTION(this << mtu);
	m_mtu = mtu;
	return true;
}

uint16_t
LearnNetDevice::GetMtu(void) const
{
	NS_LOG_FUNCTION(this);
	return m_mtu;
}

double
LearnNetDevice::GetX()
{
	return m_x;
}
double
LearnNetDevice::GetY()
{
	return m_y;
}
void LearnNetDevice::SetX(double x)
{
	m_x = x;
}
void LearnNetDevice::SetY(double y)
{
	m_y = y;
}
void LearnNetDevice::SetXY(double x, double y)
{
	m_x = x;
	m_y = y;
}

} // namespace ns3
