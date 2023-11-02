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

#include "inet/applications/udpapp/UdpPfrpApp.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/pfrpTable.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "unistd.h"
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>

using namespace std;
using namespace omnetpp;
namespace inet
{

    Define_Module(UdpPfrpApp);

    UdpPfrpApp::~UdpPfrpApp()
    {
        cancelAndDelete(selfMsg);
    }

    void UdpPfrpApp::initialize(int stage)
    {
        ApplicationBase::initialize(stage);

        if (stage == INITSTAGE_LOCAL)
        {
            numSent = 0;
            numReceived = 0;
            WATCH(numSent);
            WATCH(numReceived);

            nodeNum = par("nodeNum");
            stepTime = par("stepTime");
            zmqPort = par("zmqPort");
            routingFileName = par("routingFileName");
            messageLength = par("messageLength");
            flowRate = par("flowRate");
            sendInterval = 1 / (flowRate * 1024 * 1024 / 8 / messageLength);
            survivalTime = par("survivalTime");
            totalStep = par("totalStep");
            simMode = par("simMode");
            if (simMode == 0)
            {
                routingProtocol = "other";
            }
            else if (simMode == 1)
            {
                routingProtocol = "pfrpsa";
            }
            else if (simMode == 2)
            {
                routingProtocol = "pfrpma";
            }

            pfrpTable::initTable(nodeNum, routingFileName, zmqPort, survivalTime, totalStep, simMode);

            localPort = par("localPort");
            destPort = par("destPort");
            startTime = par("startTime");
            stopTime = par("stopTime");
            dontFragment = par("dontFragment");
            if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
                throw cRuntimeError("Invalid startTime/stopTime parameters");
            selfMsg = new cMessage("sendTimer");
            randDst = getDstNode();
        }
    }

    /**
     * @description: Randomly select a node that is not connected to the source node as the destination node.
     * @return {int} ID of destination node
     */
    int UdpPfrpApp::getDstNode()
    {
        // get the current time in microseconds
        auto now = std::chrono::system_clock::now();
        auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
        auto value = now_us.time_since_epoch();
        long micros = value.count();
        // using microseconds as random number seeds
        std::mt19937 gen(micros);
        // get source node
        string sender = getParentModule()->getFullName();
        int senderNode = atoi(sender.c_str() + 1);
        int dst = senderNode;
        while (dst == senderNode)
        {
            std::uniform_int_distribution<int> dist(0, nodeNum - 1);
            dst = dist(gen);
        }
        return dst;
    }

    void UdpPfrpApp::finish()
    {
        ApplicationBase::finish();
    }

    void UdpPfrpApp::setSocketOptions()
    {
        int timeToLive = par("timeToLive");
        if (timeToLive != -1)
            socket.setTimeToLive(timeToLive);

        int dscp = par("dscp");
        if (dscp != -1)
            socket.setDscp(dscp);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        const char *multicastInterface = par("multicastInterface");
        if (multicastInterface[0])
        {
            IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
            InterfaceEntry *ie = ift->findInterfaceByName(multicastInterface);
            if (!ie)
                throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
            socket.setMulticastOutputInterface(ie->getInterfaceId());
        }

        bool receiveBroadcast = par("receiveBroadcast");
        if (receiveBroadcast)
            socket.setBroadcast(true);

        bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
        if (joinLocalMulticastGroups)
        {
            MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
            socket.joinLocalMulticastGroups(mgl);
        }
        socket.setCallback(this);
    }

    void UdpPfrpApp::sendPacket()
    {
        string sender = getParentModule()->getFullName(); // current node(source node)
        int senderNode = atoi(sender.c_str() + 1);

        string destName = "H" + to_string(randDst);
        // The name of the packet, in the format of
        // "{source node}-{host node}-{routing protocol}-{packet number}-{current step}", such as H0-H5-pfrp-4387-0
        int sendId = pfrpTable::getInstance()->getSendId();
        string pkName = sender + "-" + destName + "-" + routingProtocol + "-" + to_string(sendId) + "-" + to_string(stepNum);
        Packet *packet = new Packet(pkName.c_str());
        sendPacketId++;

        if (dontFragment)
            packet->addTag<FragmentationReq>()->setDontFragment(true);
        const auto &payload = makeShared<ApplicationPacket>();

        payload->setChunkLength(B((int)messageLength));
        payload->setSequenceNumber(numSent);
        payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(payload);

        L3Address destAddr;
        const char *dn = destName.c_str();
        cStringTokenizer tokenizer(dn);
        const char *token = tokenizer.nextToken();
        L3AddressResolver().tryResolve(token, destAddr);
        emit(packetSentSignal, packet);

        socket.sendTo(packet, destAddr, destPort); // send packet
        numSent++;
    }

    void UdpPfrpApp::processStart()
    {
        socket.setOutputGate(gate("socketOut"));
        const char *localAddress = par("localAddress");
        socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
        setSocketOptions();

        selfMsg->setKind(SEND);
        processSend();
    }

    void UdpPfrpApp::processSend()
    {
        // No more packets are sent when the step count is exceeded
        if (stepNum <= totalStep)
        {
            if (timerStep == 0)
            {
                timerStep = simTime();
            }
            simtime_t timeC = simTime() - timerStep;
            if (timeC > stepTime)
            {
                cout << "--------  step " << stepNum << "  Node " << getParentModule()->getFullName() << "  --------" << endl;
                pfrpTable::getInstance()->pkNumRecord(sendPacketId, stepNum);
                pfrpTable::getInstance()->stepEndRecord(stepNum, simTime().dbl());

                if (simMode == 0)
                {
                    // Traditional algorithms, do not need to update the probabilistic routing table,
                    // only need to clean up the remnants of state statistics
                    pfrpTable::getInstance()->cleanPkctAndTps();
                }
                else if (simMode == 1 || simMode == 2)
                {
                    // DRL algorithms, need to update forwarding probability
                    pfrpTable::getInstance()->updateProb(stepNum);
                }

                stepNum++;
                sendPacketId = 0;
                timerStep = simTime();
            }
            sendPacket();

            cMersenneTwister *mt;
            // Traffic generation method: limit upper and lower bounds, evenly distributed
            cUniform un = cUniform(mt, 0.9 * sendInterval, 1.1 * sendInterval);
            double interval = un.draw();
            simtime_t d = simTime() + interval;
            if (stopTime < SIMTIME_ZERO || d < stopTime)
            {
                selfMsg->setKind(SEND);
                scheduleAt(d, selfMsg);
            }
            else
            {
                selfMsg->setKind(STOP);
                scheduleAt(stopTime, selfMsg);
            }
        }
    }

    void UdpPfrpApp::processStop()
    {
        socket.close();
    }

    void UdpPfrpApp::handleMessageWhenUp(cMessage *msg)
    {
        if (msg->isSelfMessage())
        {
            ASSERT(msg == selfMsg);
            switch (selfMsg->getKind())
            {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
            }
        }
        else
        {
            if (strstr(msg->getFullName(), routingProtocol.c_str()))
            {
                char msgName[50] = {0};
                strncpy(msgName, msg->getFullName(), 49);

                char *locPtr = NULL;
                char *srcNode = strtok_r(msgName, "-", &locPtr);
                char *dstNode = strtok_r(NULL, "-", &locPtr);
                char *thisNode = (char *)getParentModule()->getName();

                string thisN = thisNode;
                string dstN = dstNode;
                if (thisN == dstN)
                {
                    socket.processMessage(msg);
                }
            }
        }
    }

    void UdpPfrpApp::socketDataArrived(UdpSocket *socket, Packet *packet)
    {
        // process incoming packet
        processPacket(packet);
    }

    void UdpPfrpApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
    {
        EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;

        delete indication;
    }

    void UdpPfrpApp::socketClosed(UdpSocket *socket)
    {
        if (operationalState == State::STOPPING_OPERATION)
            startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
    }

    void UdpPfrpApp::refreshDisplay() const
    {
        ApplicationBase::refreshDisplay();

        char buf[100];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        getDisplayString().setTagArg("t", 0, buf);
    }

    void UdpPfrpApp::processPacket(Packet *pk)
    {
        emit(packetReceivedSignal, pk);

        char pkName[50] = {0};
        strncpy(pkName, pk->getFullName(), 49);

        char *locPtr = NULL;
        char *pksrc = strtok_r(pkName, "-", &locPtr) + 1;
        char *pkdst = strtok_r(NULL, "-", &locPtr) + 1;
        char *pfrpStr = strtok_r(NULL, "-", &locPtr);
        char *pktIdName = strtok_r(NULL, "-", &locPtr);
        char *stepNumStr = strtok_r(NULL, "-", &locPtr);
        int pktId = atoi(pktIdName);
        int pkStep = atoi(stepNumStr);

        // multi-agent DRL
        if (simMode == 2)
        {
            pfrpTable::getInstance()->countPktDelay(pkStep, pktId, (simTime() - pk->getCreationTime()).dbl());
            pfrpTable::getInstance()->stepOverJudge(pkStep, simTime().dbl());
        }
        else
        {
            pfrpTable::getInstance()->setDelayWithStep(pkStep, (simTime() - pk->getCreationTime()).dbl(), simTime().dbl());
        }
        numReceived++;
        delete pk;
    }
    void UdpPfrpApp::handleStartOperation(LifecycleOperation *operation)
    {
        simtime_t start = std::max(startTime, simTime());
        if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))
        {
            selfMsg->setKind(START);
            scheduleAt(start, selfMsg);
        }
    }

    void UdpPfrpApp::handleStopOperation(LifecycleOperation *operation)
    {
        cancelEvent(selfMsg);
        socket.close();
        delayActiveOperationFinish(par("stopOperationTimeout"));
    }

    void UdpPfrpApp::handleCrashOperation(LifecycleOperation *operation)
    {
        cancelEvent(selfMsg);
        socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
    }

} // namespace inet
