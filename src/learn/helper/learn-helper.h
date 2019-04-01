/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LEARN_HELPER_H
#define LEARN_HELPER_H

#include <string>
#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/trace-helper.h"

namespace ns3
{

class NetDevice;
class Node;

class LearnHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
	LearnHelper();
	virtual ~LearnHelper() {}
	void SetQueue(std::string type,
				  std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue(),
				  std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue(),
				  std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue(),
				  std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue());

	void SetDeviceAttribute(std::string name, const AttributeValue &value);

	void SetChannelAttribute(std::string name, const AttributeValue &value);

	NetDeviceContainer Install(NodeContainer c);

	NetDeviceContainer Install(Ptr<Node> n);

	NetDeviceContainer Install(std::string nName);

	void AddPosition(double x, double y)
	{
		m_xs[top] = x;
		m_ys[top] = y;
		++top;
	}

  private:
	virtual void EnablePcapInternal(std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

	virtual void EnableAsciiInternal(
		Ptr<OutputStreamWrapper> stream,
		std::string prefix,
		Ptr<NetDevice> nd,
		bool explicitFilename);

	ObjectFactory m_queueFactory;
	ObjectFactory m_channelFactory;
	ObjectFactory m_deviceFactory;
	double m_xs[16];
	double m_ys[16];
	int top;
};

} // namespace ns3

#endif /* LEARN_HELPER_H */
