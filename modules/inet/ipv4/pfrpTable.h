/***
 * @Author       : CHEN Jiawei
 * @Date         : 2023-10-18 20:28:25
 * @LastEditors  : LIN Guocheng
 * @LastEditTime : 2023-10-19 15:21:29
 * @FilePath     : /home/lgc/test/RL4Net++/modules/ipv4/pfrpTable.h
 * @Description  : This module stores the forwarding probability of the whole network,
 *                 and also acts as a statistics module for the whole network,
 *                 exchanging data with the py side via zmq communication.
 *                 Currently there is no way to use it as a local variable,
 *                 only a globally unique static object. Multi-intelligence environment
 *                 can be extended to one object per node.
 */

#include "string.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <numeric>
#include <omnetpp.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <zmq.hpp>

using namespace std;
using namespace omnetpp;

class pfrpTable
{
public:
  // Get unique static instance of probabilistic routing table.
  static pfrpTable *getInstance();

  // Initialize probabilistic routing table.
  static pfrpTable *initTable(int num, const char *file, int port, double survivalTime_v, int totalStep_v, int simMode_v);

public:
  // Initialize the intermediate variables needed to complete the probabilistic routing.
  void initiate();

  // Get next hop and the ID of the output gate forwarded by the current node to the next node.
  pair<string, int> getRoute(string path, const char *pkName, int pkByte);

  // Counts the transmission delay of all packets sent in each step.
  void setDelayWithStep(int step, double delay, double currentTime);

  /**
   * Finish the current step, calculate the average latency and packet loss for the network as a whole,
   * and send the reward to python.
   */
  void endStep(int step);

  /**
   * Send the state of the current step to python then update the probabilistic routing table
   * based on the information returned by python
   */
  void updateProb(int step);

  // Record the total number of packets sent under the current step.
  void pkNumRecord(int pkNum, int stepNum);

  /**
   * The function is called at the end of each node's step,
   * and the accumulator counts until all nodes have ended their step.
   */
  void stepEndRecord(int step, double endTime);

  // Assign the number of nodes in the network topology to nodeNum.
  void setNodeNum(int num);

  // Get the number of nodes in the network topology from nodeNum.
  int getNodeNum();

  // Select next hop for current packet based on probabilistic routing table.
  int getNextNode(int nodeId, int dstNode);

  // Convert the initial probabilistic routing table into a probability matrix.
  int **getProb(string fileName, int **Prob);

  // Get the ID of the output gate forwarded by the current node to the next node.
  int getGateId(int nodeId, int nextNode);

  // show the details of routing table and network topology
  void showInfo();

  // Assign the file path of the initial probabilistic routing table to routingFileName.
  void setRoutingFileName(const char *file);

  /**
   * Calculate the sum of the packet sizes of all the packets sent from the source node
   * to the destination node that reach the destination node.
   */
  void countPkct(int src, int dst, int pkByte);

  // Read and save variables in omnetpp.ini
  void setVals(int port, double survivalTime_v, int totalStep_v, int simMode_v);

  // Clear packets from the network topology.
  void cleanPkctAndTps();

  // Count the number of packets passing through the current node.
  void countPktInNode(string path, const char *pkName);

  // Record the delay of the packet ID pktID in the corresponding step.
  void countPktDelay(int step, int pktId, double delay);

  // Determine if the corresponding step is finished based on the current omnetpp timestamp.
  void stepOverJudge(int step, double currentTime);

  // End the current step under multi-agent DRL
  void endStepMulti(int step);

  // Get the id of the sending gate.
  int getSendId();

private:
  static pfrpTable *pTable;

private:
  pfrpTable();
  virtual ~pfrpTable();
  int **allProb; // Stores the forwarding probability between all nodes, which is updated every step.
  /**
   * Store the network topology, denoted as -1 between nodes with no links,
   * and as the corresponding link number between nodes with links present
   */
  int **topo;
  int nodeNum = 0;
  string routingFileName; // Name of the file used to initialize the forwarding probability matrix.
  int **pkct = NULL;      // Counts the amount of traffic that passes through each link in one step of time, in bytes.
  bool firstTime = true;  // Used to discard the data of the zeroth step.
  int edgeNum = 0;
  vector<vector<double>> delayWithStep;
  int *pkNumOfStep;
  bool *stepIsEnd;
  double *stepEndTime;
  bool *stepFinished;
  int *updateProbCount;
  int *stepEndRecordCount;
  double survivalTime;
  int totalStep = 0;
  int *ttlDrop;
  int *stDrop;

  vector<vector<unordered_set<int> *>> pktInNode;
  vector<unordered_map<int, double> *> pktDelay;

  int *stepPktNum; // This is used to store how many pakcets each step receives.
  int sendId = 0;  // ID used to identify the package, each package has a unique ID
  int simMode;
  int zmqPort;
  zmq::context_t zmqContext{1};
  zmq::socket_t zmqSocket{zmqContext, ZMQ_REQ};
};