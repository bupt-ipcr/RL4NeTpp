/*
 * @Author       : CHEN Jiawei
 * @Date         : 2023-10-18 20:28:25
 * @LastEditors  : LIN Guocheng
 * @LastEditTime : 2023-10-19 15:23:38
 * @FilePath     : /home/lgc/test/RL4Net++/modules/ipv4/pfrpTable.cc
 * @Description  : Probabilistic routing is implemented so that when a packet arrives at the router the next hop
 *                 is selected for forwarding based on the probabilistic routing table.
 */
#include "pfrpTable.h"

pfrpTable *pfrpTable::pTable = NULL;

pfrpTable::pfrpTable()
{
}

pfrpTable::~pfrpTable()
{
}

/**
 * @description: Get unique static instance of probabilistic routing table.
 * @return {*pfrpTable} probabilistic routing table
 */
pfrpTable *pfrpTable::getInstance()
{
    if (!pTable)
    {
        throw "pTable is not initiated";
    }
    return pTable;
}

/**
 * @description: Initialize probabilistic routing table.
 * @param {int} num                 node num in network topology
 * @param {char} *file              file path to initial probabilistic routing table
 * @param {int} port                communication port for ZMQ
 * @param {double} survivalTime_v   survival time, set in omnetpp.ini
 * @param {int} totalStep_v         total step, set in omnetpp.ini
 * @param {int} simMode_v           simulation mode, set in omnetpp.ini
 * @return {*pfrpTable}             Probabilistic routing table for completion of initialization.
 */
pfrpTable *pfrpTable::initTable(int num, const char *file, int port, double survivalTime_v, int totalStep_v, int simMode_v)
{
    if (!pTable)
    {
        pTable = new pfrpTable();
        pTable->setNodeNum(num);
        pTable->setRoutingFileName(file);
        pTable->setVals(port, survivalTime_v, totalStep_v, simMode_v);
        pTable->initiate();
    }
    return pTable;
}

/**
 * @description: Initialize the intermediate variables needed to complete the probabilistic routing.
 * @return {*} None
 */
void pfrpTable::initiate()
{
    string connectAddr = "tcp://localhost:" + to_string(zmqPort);
    zmq_connect((void *)zmqSocket, connectAddr.c_str());

    topo = (int **)malloc(nodeNum * sizeof(int *));
    memset(topo, 0, nodeNum);
    int *tmp = (int *)malloc(nodeNum * nodeNum * sizeof(int));
    memset(tmp, 0, nodeNum * nodeNum);
    for (int i = 0; i < nodeNum; i++)
    {
        topo[i] = tmp + nodeNum * i;
    }

    allProb = (int **)malloc(nodeNum * sizeof(int *));
    memset(allProb, 0, nodeNum * sizeof(int *));
    tmp = (int *)malloc(nodeNum * nodeNum * sizeof(int));
    memset(tmp, 0, nodeNum * nodeNum * sizeof(int));
    for (int i = 0; i < nodeNum; i++)
    {
        allProb[i] = tmp + nodeNum * i;
    }
    allProb = getProb(routingFileName, allProb);

    pkct = (int **)malloc(nodeNum * sizeof(int *));
    memset(pkct, 0, nodeNum * sizeof(int *));
    tmp = (int *)malloc(nodeNum * nodeNum * sizeof(int));
    memset(tmp, 0, nodeNum * nodeNum * sizeof(int));
    for (int i = 0; i < nodeNum; i++)
    {
        pkct[i] = tmp + nodeNum * i;
    }

    stepIsEnd = (bool *)malloc(totalStep * sizeof(bool));
    memset(stepIsEnd, false, totalStep * sizeof(bool));

    stepFinished = (bool *)malloc(totalStep * sizeof(bool));
    memset(stepFinished, false, totalStep * sizeof(bool));

    stepEndTime = (double *)malloc(totalStep * sizeof(double));
    memset(stepEndTime, 0, totalStep * sizeof(double));

    stepEndRecordCount = (int *)malloc(totalStep * sizeof(int));
    memset(stepEndRecordCount, 0, totalStep * sizeof(int));

    updateProbCount = (int *)malloc(totalStep * sizeof(int));
    memset(updateProbCount, 0, totalStep * sizeof(int));

    pkNumOfStep = (int *)malloc(totalStep * sizeof(int));
    memset(pkNumOfStep, 0, totalStep * sizeof(int));

    ttlDrop = (int *)malloc(totalStep * sizeof(int));
    memset(ttlDrop, 0, totalStep * sizeof(int));

    stDrop = (int *)malloc(totalStep * sizeof(int));
    memset(stDrop, 0, totalStep * sizeof(int));

    for (int i = 0; i < nodeNum; i++)
        for (int j = i; j < nodeNum; j++)
            if (topo[i][j] >= 0)
                edgeNum++;

    // multi-agent DRL
    if (simMode == 2)
    {

        for (int i = 0; i < totalStep; i++)
        {
            vector<unordered_set<int> *> nvec;
            for (int j = 0; j < nodeNum; j++)
            {
                unordered_set<int> *nuset = new unordered_set<int>;
                nvec.push_back(nuset);
            }
            pktInNode.push_back(nvec);

            unordered_map<int, double> *numap = new unordered_map<int, double>;
            pktDelay.push_back(numap);
        }

        stepPktNum = (int *)malloc(totalStep * sizeof(int));
        memset(stepPktNum, 0, totalStep * sizeof(int));
    }
}

/**
 * @description: Assign the number of nodes in the network topology to nodeNum.
 * @param {int} num     the number of nodes in the network topology
 * @return {*}          None
 */
void pfrpTable::setNodeNum(int num)
{
    nodeNum = num;
}

/**
 * @description: Get the number of nodes in the network topology from nodeNum.
 * @return {int} the number of nodes in the network topology
 */
int pfrpTable::getNodeNum()
{
    return nodeNum;
}

/**
 * @description: Convert the initial probabilistic routing table into a probability matrix.
 * @param {string} fileName     file path to initial probabilistic routing table
 * @param {int} **Prob          initial probabilistic routing matrix
 * @return {**pfrpTable}        updated probabilitstic routing matrix
 */
int **pfrpTable::getProb(string fileName, int **Prob)
{
    ifstream myfile(fileName);

    for (int i = 0; i < nodeNum; i++)
    {
        for (int j = 0; j < nodeNum; j++)
        {
            string od_prob;
            getline(myfile, od_prob, ',');
            Prob[i][j] = atoi(od_prob.c_str());
        }
    }

    int edgeCount = 0;
    for (int i = 0; i < nodeNum; i++)
    {
        for (int j = i; j < nodeNum; j++)
        {

            if (Prob[i][j])
            {
                topo[i][j] = edgeCount * 2;
                topo[j][i] = edgeCount * 2 + 1;
                edgeCount++;
            }
            else
            {
                topo[i][j] = -1;
                topo[j][i] = -1;
            }
        }
    }
    myfile.close();
    return Prob;
}

/***
 * @description: Select next hop for current packet based on probabilistic routing table.
 * @param {int} nodeId      ID of current node
 * @param {int} dstNode     ID of destination of current packet
 * @return {int}            ID of the next-hop node that can be selected
 */
int pfrpTable::getNextNode(int nodeId, int dstNode)
{
    if (allProb[nodeId][dstNode])
    {
        return dstNode;
    }
    else
    {
        vector<int> candidateNodes;
        vector<int> candidateProbs;
        int probSum = 0;
        for (int i = 0; i < nodeNum; i++)
        {
            // if the point with id i is adjacent to the current point and up to the dstnode node
            if (allProb[nodeId][i])
            {
                candidateNodes.push_back(i);
                candidateProbs.push_back(allProb[nodeId][i]);
                probSum += allProb[nodeId][i];
            }
        }
        int randProb = int(rand() % probSum);

        for (int i = 0; i < candidateNodes.size(); i++)
        {
            int curProb = 0;
            for (int j = 0; j <= i; j++)
                curProb += candidateProbs[j];
            if (randProb <= curProb)
                return candidateNodes[i];
        }
    }
}

/**
 * @description: Get the ID of the output gate forwarded by the current node to the next node.
 * @param {int} nodeId      ID of current node
 * @param {int} nextNode    ID of destination of current packet
 * @return {int}            ID of the output gate
 */
int pfrpTable::getGateId(int nodeId, int nextNode)
{

    int gateId = 0;

    // links in the environment are bi-directional links
    for (int i = 0; i < nodeNum; i++)
    {
        if (allProb[nodeId][i])
        {
            if (i == nextNode)
                break;
            else
                gateId++;
        }
    }
    // The first bit of ift has a lo0, so +1.
    gateId++;
    return gateId;
}

/**
 * @description: Get next hop and the ID of the output gate forwarded by the current node to the next node.
 * @param {string} path         routing path
 * @param {char} *pkName        name of the packet
 * @param {int} pkByte          size of the packet, unit: Byte
 * @return {pair<string, int>}  R or H + destination Node ID, ID of the output gate
 */
pair<string, int> pfrpTable::getRoute(string path, const char *pkName, int pkByte)
{

    if (!pTable)
    {
        throw "getRoute";
    }

    char pathCpy[50] = {0};
    strncpy(pathCpy, path.c_str(), 49);
    char *locPtr = NULL;
    char *networkName = strtok_r(pathCpy, ".", &locPtr);
    char *thisNodeName = strtok_r(NULL, ".", &locPtr);
    int thisNodeId = atoi(thisNodeName + 1);

    // a pkName example: H0-H5-pfrp-4387-0
    char pkNameCpy[50] = {0};
    strncpy(pkNameCpy, pkName, 49);
    char *locPtr_1 = NULL;
    char *srcName = strtok_r(pkNameCpy, "-", &locPtr_1);
    char *dstName = strtok_r(NULL, "-", &locPtr_1);
    int dstNodeId = atoi(dstName + 1);

    pair<string, int> p;

    if (thisNodeName[0] == 'H')
    {
        // first hop: from host to router
        p.first = "R" + to_string(thisNodeId);
        p.second = 1;
        return p;
    }

    if (thisNodeId == dstNodeId)
    {
        // final hop: from router to host
        p.first = "H" + to_string(dstNodeId);
        int gid = 0;
        for (int i = 0; i < nodeNum; i++)
        {
            if (allProb[dstNodeId][i])
            {
                gid++;
            }
        }
        // Ift has a lo0 in the first place so it should be +1,
        // router pointing to host is the last interface so it should be +1 again
        // counting from 0 so it should be -1
        p.second = gid + 1;
        return p;
    }

    int nextNode = getNextNode(thisNodeId, dstNodeId);
    p.first = "R" + to_string(nextNode);
    p.second = getGateId(thisNodeId, nextNode);
    countPkct(thisNodeId, nextNode, pkByte);

    return p;
}

/**
 * @description: show the details of routing table and network topology
 * @return {*} None
 */
void pfrpTable::showInfo()
{
    cout << endl;
    cout << "allProb:" << endl;
    for (int ii = 0; ii < nodeNum; ii++)
    {
        for (int jj = 0; jj < nodeNum; jj++)
        {
            cout << allProb[ii][jj] << " ";
        }
        cout << endl;
    }

    cout << endl;
    cout << "topo:" << endl;
    for (int ii = 0; ii < nodeNum; ii++)
    {
        for (int jj = 0; jj < nodeNum; jj++)
        {
            cout << topo[ii][jj] << " ";
        }
        cout << endl;
    }

    cout << endl;
    cout << "edgeNum:  " << edgeNum << endl;
    cout << endl;
    cout << "totalStep:  " << totalStep << endl;
    cout << endl;
}

/**
 * @description: Assign the file path of the initial probabilistic routing table to routingFileName.
 * @param {char} *file  the file path of the initial probabilistic routing table
 * @return {*} None
 */
void pfrpTable::setRoutingFileName(const char *file)
{
    string s(file);
    routingFileName = s;
}

/**
 * @description: Calculate the sum of the packet sizes of all the packets sent from the source node to
 *               the destination node that reach the destination node.
 * @param {int} src     source node from which the packet is sent
 * @param {int} dst     destination node of the packet
 * @param {int} pkByte  size of the packet, unit: Byte
 * @return {*} None
 */
void pfrpTable::countPkct(int src, int dst, int pkByte)
{
    pkct[src][dst] += pkByte;
}

/**
 * @description: Read and save variables in omnetpp.ini
 * @param {int} port                port of ZMQ communication
 * @param {double} survivalTime_v   survival time in omnetpp.ini
 * @param {int} totalStep_v         total step in omnetpp.ini
 * @param {int} simMode_v           simulation mode in omnetpp.ini
 * @return {*}
 */
void pfrpTable::setVals(int port, double survivalTime_v, int totalStep_v, int simMode_v)
{
    zmqPort = port;
    survivalTime = survivalTime_v;
    totalStep = totalStep_v;
    simMode = simMode_v;
}

/**
 * @description: Clear packets from the network topology.
 * @return {*} None
 */
void pfrpTable::cleanPkctAndTps()
{

    for (int i = 0; i < nodeNum; i++)
        for (int j = 0; j < nodeNum; j++)
        {
            pkct[i][j] = 0;
        }
}

/**
 * @description: Counts the transmission delay of all packets sent in each step.
 * @param {int} step                step for which all packets have been sent and received
 * @param {double} delay            delay statistics in the step
 * @param {double} currentTime      current timestamp in omnetpp environment
 * @return {*} None
 */
void pfrpTable::setDelayWithStep(int step, double delay, double currentTime)
{
    if (!pTable)
    {
        throw "setDelayWithStep";
    }
    if (delayWithStep.size() < (step + 1))
    {
        vector<double> delayVec;
        delayVec.push_back(delay);
        delayWithStep.push_back(delayVec);
    }
    else
    {
        delayWithStep[step].push_back(delay);

        if (stepIsEnd[step] && (!stepFinished[step]))
        {
            if (delayWithStep[step].size() == pkNumOfStep[step])
            {
                endStep(step);
                delayWithStep[step].clear();
            }
        }

        for (int formerStep = 0; formerStep <= step; formerStep++)
        {
            double timePast = currentTime - stepEndTime[formerStep];
            if (stepIsEnd[formerStep] && (timePast >= survivalTime) && (!stepFinished[formerStep]))
            {
                endStep(formerStep);
                delayWithStep[formerStep].clear();
            }
        }
    }
}

/**
 * @description: Send the state of the current step to python
 *               then update the probabilistic routing table based on the information returned by python
 * @param {int} step    the current step that the status message to be sent
 * @return {*} None
 */
void pfrpTable::updateProb(int step)
{
    // send state message and receive reward message
    if (!pTable)
    {
        throw "updateProb";
    }

    updateProbCount[step]++;
    if (updateProbCount[step] == nodeNum)
    {
        // the last node to go into next update step
        string stateStr;
        stateStr += "s@@" + to_string(step) + "@@";
        for (int i = 0; i < nodeNum; i++)
            for (int j = 0; j < nodeNum; j++)
            {
                if (i == nodeNum - 1 && j == nodeNum - 1)
                    stateStr += to_string(double(pkct[i][j]) / 1024 / 1024);
                else
                    stateStr += to_string(double(pkct[i][j]) / 1024 / 1024) + ",";
            }
        cleanPkctAndTps();

        const char *reqData = stateStr.c_str();
        const size_t reqLen = strlen(reqData);
        zmq::message_t request{reqLen};
        memcpy(request.data(), reqData, reqLen);
        zmqSocket.send(request);

        zmq::message_t reply;
        zmqSocket.recv(&reply);
        char *buffer = new char[reply.size() + 1];
        memset(buffer, 0, reply.size() + 1);
        memcpy(buffer, reply.data(), reply.size());

        // single-agent DRL
        if (simMode == 1)
        {
            // Update for single-agent DRL
            // Get the link weights for twice the number of network links and assemble them into a probability matrix on the omnetpp side.
            char *od_prob;
            double *weights = new double[edgeNum * 2];
            for (int i = 0; i < edgeNum * 2; i++)
            {
                if (i == 0)
                {
                    od_prob = strtok(buffer, ",");
                }
                else
                {
                    od_prob = strtok(NULL, ",");
                }
                weights[i] = atof(od_prob);
            }

            for (int i = 0; i < nodeNum; i++)
            {
                double totalWeight = 0.0;
                for (int j = 0; j < nodeNum; j++)
                {
                    if (topo[i][j] >= 0)
                    {
                        totalWeight += weights[topo[i][j]];
                    }
                }
                for (int j = 0; j < nodeNum; j++)
                {
                    if (topo[i][j] >= 0)
                    {
                        int prob = (int)(weights[topo[i][j]] / totalWeight * 100);
                        if (prob < 1)
                        {
                            prob = 1;
                        }
                        allProb[i][j] = prob;
                    }
                }
            }
            delete weights;
        }
        // multi-agent
        else if (simMode == 2)
        {
            // The probability matrix is calculated on the python side and passed directly to the receiver.
            char *newProb;
            for (int i = 0; i < nodeNum; i++)
            {
                for (int j = 0; j < nodeNum; j++)
                {
                    if (i == 0 && j == 0)
                    {
                        newProb = strtok(buffer, ",");
                    }
                    else
                    {
                        newProb = strtok(NULL, ",");
                    }
                    allProb[i][j] = atoi(newProb);
                }
            }
        }
        sendId = 0; // reset packer ID at the start of next step
        showInfo();
        delete buffer;
    }
}

/**
 * @description: Record the total number of packets sent under the current step.
 * @param {int} pkNum       the number of packets sent under the current step
 * @param {int} stepNum     current step to be recorded
 * @return {*} None
 */
void pfrpTable::pkNumRecord(int pkNum, int stepNum)
{
    pkNumOfStep[stepNum] += pkNum;
}

/**
 * @description: Finish the current step,
 *               calculate the average latency and packet loss for the network as a whole,
 *               and send the reward to python
 * @param {int} step    the step to be finished
 * @return {*} None
 */
void pfrpTable::endStep(int step)
{
    if (firstTime)
    {
        firstTime = false;
    }
    else
    {
        double sum = 0.0;
        for (int i = 0; i < delayWithStep[step].size(); i++)
        {
            sum += delayWithStep[step][i];
        }
        double avgDelay = sum / delayWithStep[step].size();

        double lossRate = 1.0 - (double)(delayWithStep[step].size()) / (double)(pkNumOfStep[step]);
        string reward = to_string(avgDelay) + "," + to_string(lossRate);
        string reqStr = "r@@" + to_string(step) + "@@" + reward;

        cout << "---- reward ---- " << reqStr << endl;

        const char *reqData = reqStr.c_str();
        const size_t reqLen = strlen(reqData);
        zmq::message_t request{reqLen};
        memcpy(request.data(), reqData, reqLen);
        zmqSocket.send(request);

        zmq::message_t reply;
        zmqSocket.recv(&reply);
    }
    stepFinished[step] = true;
}

/**
 * @description: the function is called at the end of each node's step,
 *               and the accumulator counts until all nodes have ended their step.
 * @param {int} step        the corresponding step when the function wakes up
 * @param {double} endTime  the end time of the current step
 * @return {*} None
 */
void pfrpTable::stepEndRecord(int step, double endTime)
{
    if (!pTable)
    {
        throw "stepEndRecord";
    }

    stepEndRecordCount[step]++;
    if (stepEndRecordCount[step] == nodeNum)
    {
        stepIsEnd[step] = true;
        stepEndTime[step] = endTime;
    }
}

/**
 * @description: Count the number of packets passing through the current node.
 * @param {string} path     routing path for arriving packets
 * @param {char} *pkName    name of the arriving packet
 * @return {*} None
 */
void pfrpTable::countPktInNode(string path, const char *pkName)
{
    char pathCpy[50] = {0};
    strncpy(pathCpy, path.c_str(), 49);
    char *locPtr = NULL;
    char *networkName = strtok_r(pathCpy, ".", &locPtr);
    char *thisNodeName = strtok_r(NULL, ".", &locPtr);

    // only passed routers are counted
    if (thisNodeName[0] == 'H')
    {
        return;
    }

    int thisNodeId = atoi(thisNodeName + 1);

    // an example of pkName: H0-H5-pfrp-4387-0
    char pkNameCpy[50] = {0};
    strncpy(pkNameCpy, pkName, 49);
    char *locPtr_1 = NULL;
    char *srcName = strtok_r(pkNameCpy, "-", &locPtr_1);
    char *dstName = strtok_r(NULL, "-", &locPtr_1);
    char *ProtName = strtok_r(NULL, "-", &locPtr_1);
    char *pktIdName = strtok_r(NULL, "-", &locPtr_1);
    char *stepNumName = strtok_r(NULL, "-", &locPtr_1);

    int pktId = atoi(pktIdName);
    int stepNum = atoi(stepNumName);
    (*pktInNode[stepNum][thisNodeId]).insert(pktId);
}

/**
 * @description: Record the delay of the packet ID pktID in the corresponding step.
 * @param {int} step        step of the packet to be counted
 * @param {int} pktId       ID of the packet to be counted
 * @param {double} delay    delay of the packet to be counted
 * @return {*} None
 */
void pfrpTable::countPktDelay(int step, int pktId, double delay)
{
    (*pktDelay[step])[pktId] = delay;
}

/**
 * @description: Determine if the corresponding step is finished based on the current omnetpp timestamp.
 * @param {int} step            step to be judged
 * @param {double} currentTime  the current omnetpp timestamp
 * @return {*} None
 */
void pfrpTable::stepOverJudge(int step, double currentTime)
{
    stepPktNum[step]++;
    if (stepIsEnd[step] && (!stepFinished[step]))
    {

        if (stepPktNum[step] == pkNumOfStep[step])
        {
            endStepMulti(step);
        }
    }

    for (int formerStep = 0; formerStep <= step; formerStep++)
    {
        double timePast = currentTime - stepEndTime[formerStep];
        if (stepIsEnd[formerStep] && (timePast >= survivalTime) && (!stepFinished[formerStep]))
        {
            endStepMulti(formerStep);
        }
    }
}

/**
 * @description: End the current step under multi-agent DRL
 * @param {int} step    the step to be finished
 * @return {*} None
 */
void pfrpTable::endStepMulti(int step)
{
    if (firstTime)
    {
        firstTime = false;
    }
    else
    {
        vector<int> pktPass(nodeNum, 0);
        vector<int> pktArrive(nodeNum, 0);
        vector<vector<double>> delays;
        for (int i = 0; i < nodeNum; i++)
        {
            vector<double> nvec;
            for (int pktId : (*pktInNode[step][i]))
            {
                if ((*pktDelay[step]).count(pktId))
                {
                    nvec.push_back((*pktDelay[step])[pktId]);
                    pktArrive[i]++;
                }
                pktPass[i]++;
            }

            delays.push_back(nvec);
        }

        string reqStr = "r@@" + to_string(step) + "@@";
        for (int i = 0; i < nodeNum; i++)
        {
            double sum = 0.0;
            for (int j = 0; j < delays[i].size(); j++)
            {
                sum += delays[i][j];
            }
            double avgDelay = (delays[i].size() == 0) ? 0.0 : (sum / delays[i].size());

            double lossRate = 1.0 - (double)(pktArrive[i]) / (double)(pktPass[i]);

            reqStr += to_string(avgDelay) + "," + to_string(lossRate) + "/";
        }
        reqStr.pop_back(); // remove final '/'

        cout << endl;
        cout << " -------- reward -------- " << endl;
        cout << endl
             << endl
             << reqStr << endl
             << endl;

        for (int i = 0; i < nodeNum; i++)
        {
            pktInNode[step][i]->clear();
        }
        pktDelay[step]->clear();

        const char *reqData = reqStr.c_str();
        const size_t reqLen = strlen(reqData);
        zmq::message_t request{reqLen};
        memcpy(request.data(), reqData, reqLen);
        zmqSocket.send(request);

        zmq::message_t reply;
        zmqSocket.recv(&reply);
    }
    stepFinished[step] = true;
}

/**
 * @description: Get the id of the sending gate.
 * @return {*} None
 */
int pfrpTable::getSendId()
{
    return sendId++;
}
