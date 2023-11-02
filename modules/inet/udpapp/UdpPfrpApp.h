//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_UDPPFRPAPP_H
#define __INET_UDPPFRPAPP_H

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "inet/common/INETDefs.h"
#include <typeinfo>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

using namespace std;

namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API UdpPfrpApp : public ApplicationBase, public UdpSocket::ICallback {
  protected:
    enum SelfMsgKinds { START = 1,
                        SEND,
                        STOP };

    // parameters
    std::vector<std::string> destAddressStr;
    int localPort = -1, destPort = -1;
    simtime_t startTime;
    simtime_t stopTime;
    bool dontFragment = false;
    const char *packetName = nullptr;

    int nodeNum;
    int sendPacketId = 0;
    const char *routingFileName;
    string routingProtocol;
    double dv;
    double tpv;
    double nuv;
    int stepTime;
    int zmqPort;
    int messageLength;
    double flowRate;
    double sendInterval;
    int ttl;
    int totalStep;
    double survivalTime;
    int simMode;
    int fixedDst;

    UdpSocket socket;
    cMessage *selfMsg = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // chooses random destination address
    virtual void sendPacket();
    virtual void processPacket(Packet *msg);
    virtual void setSocketOptions();

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;

    int getDstNode();

  public:
    UdpPfrpApp() {}
    ~UdpPfrpApp();
    simtime_t oTime = 0;
    simtime_t timerStep = 0;
    int stepNum = 0;
};

} // namespace inet

#endif // ifndef __INET_UDPPFRPAPP_H
