# ns-3 Channel_Module
This repo records my learning on ns-3. I simply designed a channel module and implement a Netdevice using this channel. I developed this project to learn about the development & usage of ns-3  to support my study on NR channel modules developing on ns-3.

### Run the code
Clone from git  
``` git clone https://github.com/Mauriyin/ns-3-Channel_Module.git ```  
Copy code to your local ns-3-dev  
```cp -r NS3-Channel_Module\* ${YOUR_NS3_DEV}```  
Checkout to your ns-3-dev  
```cd ${YOUR_NS3_DEV}```  
Run test     
`./waf --run scratch/test_channel`

### Code design
#### learn.h
The definition of the 2 basic class: **LearnChannel** & **LearnNetDevice**
##### LearnChannel
The channel module that implemented by myself.
As a simple test class, so I only assure the delay is related to the distance, using the distance*factor to calculate the delay.
```
delay =  m_delay_fac * GetDist(n1, n2);
```
##### LearnNetDevice
Implement a basic Net device to transmit the packets and receive the packets. Using the point-to-point-net-device as a reference to send and receive packets, and added features to support the channel module.

### Test result
Test script is under the scratch, test-channel.cc. It added two devices and test the channel moudle and device set up by mysel. The result shown as below:
![test result](https://i.loli.net/2019/04/01/5ca216efc0c10.png)
