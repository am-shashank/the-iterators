# Distributed-Chat-Server

## Introduction
As part of the project, we will be implementing a fully ordered multicast chat server with  a central sequencer/leader (chosen from among the current group members). The leader will be responsible for sequencing messages as well as determining who the current active clients are, detecting eliminating any inactive/crashed users from the chat (and making sure that the remaining clients have an up to date list of those currently in the chat). A new client can add to the system by sending request either to the leader or any active client. Each client will communicate with the leader from time to time. Along with this, various recovery mechanisms will be implemented, like message failure recovery, election of new leader in case the leader crashes or exits, etc.

## Various Modules
- Naming Module
- Total Ordering/ Sequencer Module
- Heartbeat Module
- Recovery Module
  - Message Failure Recovery Module
  - Election Module
