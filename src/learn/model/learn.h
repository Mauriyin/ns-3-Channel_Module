/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LEARN_H
#define LEARN_H
#include "ns3/channel.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/mac48-address.h"
#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/header.h"

namespace ns3
{


class LearnNetDevice;
class Packet;

class LearnChannel : public Channel
{
  public:
	static TypeId GetTypeId(void);
	//construct the channel
	LearnChannel();
	//attach the device to this channel
	void Attach(Ptr<LearnNetDevice> device);
	//start to send packet to src at txTime
	virtual bool TransmitStart(Ptr<const Packet> p, Ptr<LearnNetDevice> src, Time txTime);
	//device number attached to this device
	virtual std::size_t GetNDevices(void) const;
	//get the i device of the attached learn devices
	Ptr<LearnNetDevice> GetLearnDevice(std::size_t i) const;
	//get the i device of the attached devices
	virtual Ptr<NetDevice> GetDevice(std::size_t i) const;

  protected:
	//get the delay of channel from n1 to n2
	Time GetDelay(Ptr<LearnNetDevice> n1, Ptr<LearnNetDevice> n2) const;
	//get the delay factor of the channel
	Time GetDelayFac(void) const;

  private:
	//get the distance from n1 to n2
	virtual double GetDist(Ptr<LearnNetDevice> n1, Ptr<LearnNetDevice> n2) const;
	//max attached device number for each channel
	static const std::size_t N_DEVICES = 16;
	//the relationship between time and distance
	Time m_delay_fac;
	//number of devices attached to this channel
	std::size_t m_nDevices;

	TracedCallback<Ptr<const Packet>, Ptr<NetDevice>, Ptr<NetDevice>, Time, Time> m_txrxLearn;
	//devices attach to this channel
	Ptr<LearnNetDevice> m_devices[N_DEVICES];
	//the distance between the attached devices
	double m_dist[N_DEVICES][N_DEVICES];
};

//////////////////////////////////////////////////////////////////////////

template <typename Item>
class Queue;
class LearnChannel;
class ErrorModel;

class LearnNetDevice : public NetDevice
{
  public:
	static TypeId GetTypeId(void);

	LearnNetDevice();
	virtual ~LearnNetDevice();

	void SetDataRate(DataRate bps);

	void SetInterframeGap(Time t);
	//attach device to channel
	bool Attach(Ptr<LearnChannel> ch);

	void SetQueue(Ptr<Queue<Packet>> queue);

	Ptr<Queue<Packet>> GetQueue(void) const;

	void SetReceiveErrorModel(Ptr<ErrorModel> em);
	//receive callback handler
	void Receive(Ptr<Packet> p, Ptr<LearnNetDevice> src);

	virtual double GetX();

	virtual double GetY();

	virtual void SetX(double x);
	
	virtual void SetY(double y);
	
	virtual void SetXY(double x, double y);

	////////////////////////////////////////////////////////////////

	virtual void SetIfIndex(const uint32_t index);

	virtual uint32_t GetIfIndex(void) const;

	virtual Ptr<Channel> GetChannel(void) const;

	virtual void SetAddress(Address address);
	
	virtual Address GetAddress(void) const;

	virtual bool SetMtu(const uint16_t mtu);
	
	virtual uint16_t GetMtu(void) const;
	
	virtual bool IsLinkUp(void) const;
	
	virtual void AddLinkChangeCallback(Callback<void> callback);
	
	virtual bool IsBroadcast(void) const;
	
	virtual Address GetBroadcast(void) const;
	
	virtual bool IsMulticast(void) const;
	
	virtual Address GetMulticast(Ipv4Address multicastGroup) const;
	址
	virtual Address GetMulticast(Ipv6Address addr) const;
	
	virtual bool IsBridge(void) const;
	
	virtual bool IsPointToPoint(void) const;
	//send packet to dest
	virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

	virtual bool SendFrom(Ptr<Packet> packet, const Address &source, const Address &dest, uint16_t protocolNumber);

	virtual Ptr<Node> GetNode(void) const;

	virtual void SetNode(Ptr<Node> node);

	virtual bool NeedsArp(void) const;

	virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);

	virtual void SetPromiscReceiveCallback(PromiscReceiveCallback cb);

	virtual bool SupportsSendFrom(void) const;
	////////////////////////////////////////////////////////////////

  private:
	LearnNetDevice &operator=(const LearnNetDevice &o);
	LearnNetDevice(const LearnNetDevice &o);
	//reset the device
	virtual void DoDispose(void);

  private:
	bool TransmitStart(Ptr<Packet> p);
	void TransmitComplete(void);
	void NotifyLinkUp(void);
	enum TxMachineState
	{
		READY,
		BUSY
	};
	//tx state
	TxMachineState m_txMachineState;
	//datarate
	DataRate m_bps;
	//interframe of this device
	Time m_tInterframeGap;
	//attached channel
	Ptr<LearnChannel> m_channel;
	//tx queue
	Ptr<Queue<Packet>> m_queue;
	Ptr<ErrorModel> m_receiveErrorModel;

	TracedCallback<Ptr<const Packet>> m_macTxTrace;
	TracedCallback<Ptr<const Packet>> m_macTxDropTrace;
	TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;
	TracedCallback<Ptr<const Packet>> m_macRxTrace;
	TracedCallback<Ptr<const Packet>> m_macRxDropTrace;
	TracedCallback<Ptr<const Packet>> m_phyTxBeginTrace;
	TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;
	TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;
	TracedCallback<Ptr<const Packet>> m_phyRxBeginTrace;
	TracedCallback<Ptr<const Packet>> m_phyRxEndTrace;
	TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;
	TracedCallback<Ptr<const Packet>> m_snifferTrace;
	TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;
	//attached node
	Ptr<Node> m_node;
	//Mac Address
	Mac48Address m_address;
	//call back
	NetDevice::ReceiveCallback m_rxCallback;
	
	NetDevice::PromiscReceiveCallback m_promiscCallback;
	
	uint32_t m_ifIndex;
	
	bool m_linkUp;
	
	TracedCallback<> m_linkChangeCallbacks;
	//default MTU
	static const uint16_t DEFAULT_MTU = 1500;
	//mtu
	uint32_t m_mtu;
	//packet
	Ptr<Packet> m_currentPkt;
	//position
	double m_x;
	double m_y;
};

} // namespace ns3

#endif /* LEARN_H */
