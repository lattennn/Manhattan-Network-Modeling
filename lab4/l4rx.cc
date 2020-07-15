#define FSM_DEBUG // To view state changes in console.
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l4Msg_m.h"

using namespace omnetpp;

long rx_SEQ[25]; // Array to save last received sequence number, the index corresponds to the source.

class l4_rx : public cSimpleModule
{
  private:
    int numRxed, ind, kind, ack_seq, id_last_1, id_last_0, ack_id_aux;
    // number of received messages, module index, kind to be set to messages and seq of ACK to be sent.
    // Also, id for the last received seq 1 and seq 0 messages, and an temporary placeholder variable are used to keep fsm synchronized.
    simtime_t Module21_Avg; // Final delivery delay we're interested in finding.
    l4Msg *ACK_rxed; // The message sent to TX to confirm the arrival of acks.
    l4Msg *start; // Selfmessage to start simulation.
    simtime_t delay_total;  // Total delay of all messages.
    double gen_ack; // Number of generated ACKs.

    cFSM rx_fsm; // Our FSM

    // States
    enum {
        INIT = 0,
        WAIT_FOR_DATA_0 = FSM_Steady(1), // Waiting for data 0
        WAIT_FOR_DATA_1 = FSM_Steady(2), // Waiting for data 1
        FINISH = FSM_Steady(3), // Finish state
        SEND_TX_AT_0 = FSM_Transient(1), // When waiting for data 0, receiving an ACK
        SEND_TX_AT_1 = FSM_Transient(2), // When waiting for data 1, receiving an ACK
        FROM_0_TO_1 = FSM_Transient(3), // When waiting for data 0, receiving data 0.
        SEND_BACK_ACK_IN_0 = FSM_Transient(4), // When waiting for data 0, receiving data 1.
        FROM_1_TO_0 = FSM_Transient(5), // When waiting for data 1, receiving data 1.
        SEND_BACK_ACK_IN_1 = FSM_Transient(6), // When waiting for data 1, receiving data 0.
        SEND_TX_AT_FINISH = FSM_Transient(7), // After having finished, keep sending ACK received.
    };

  public:
    virtual ~l4_rx();

  protected:
    virtual void initialize() override;
    // Initialization of variables and scheduling of selfmessage to start.
    virtual void handleMessage(cMessage *msg) override;
    // Handles actions to perform when receiving a message.
    virtual void generateACK(l4Msg *msg);
    // Message to generate acks, message given contains seq and ack info.
    virtual void finish() override;
    // Function to be called when simulaton is complete.
};

Define_Module(l4_rx);


l4_rx::~l4_rx()
{
   delete start; // Delete starting message.
}


void l4_rx::initialize()
{
    // Set variables to 0.
    numRxed = id_last_1 = id_last_0 = 0;
    WATCH(numRxed); // Debugging purposes.
    delay_total = 0;
    Module21_Avg = 0;
    gen_ack = 0;
    ind = getParentModule() -> getIndex(); // Index of module
    rx_fsm.setName("rx_fsm"); // Naming fsm
    start = new l4Msg("start"); // Creating new self message.
    scheduleAt(simTime(), start); // Scheduling to current simulation time.
}

void l4_rx::handleMessage(cMessage *msg)
{
    // Kind 1 = Data 0
    // Kind 2 = Data 1
    // Kind 3 = Ack 0
    // Kind 4 = Ack 1
    // Kind 5 = Ack_rxed 0
    // Kind 6 = Ack_rxed 1

    l4Msg *ttmsg = check_and_cast<l4Msg *>(msg);

    FSM_Switch(rx_fsm) {
        case FSM_Exit(INIT):
            FSM_Goto(rx_fsm, WAIT_FOR_DATA_0); // Start at waiting for data 0.
            break;

        case FSM_Enter(WAIT_FOR_DATA_0):
            // Do nothing at entrance.
            break;

        case FSM_Exit(WAIT_FOR_DATA_0):
            if (ttmsg-> getKind() == 1){ // If we get Data 0
                //We get the delivery delay of the message
                l4Msg *ttmsg = check_and_cast<l4Msg *>(msg); // We cast the message to our data type.
                if (id_last_0 >= ttmsg->getId()){
                    // If the id is from an old message (smaller Id), we just send ack, no statistics are performed.
                    FSM_Goto(rx_fsm,SEND_BACK_ACK_IN_0);
                } else {
                    id_last_0 = ttmsg -> getId(); // Else, we update the last valid id.
                    if(numRxed != 6900){ // If we have not received 6900 messages
                        if (ind == 21){ // If we're module G+2,
                            if (rx_SEQ[ttmsg->getSource()] != 0){
                                // And if in index of source of data message the last received sequence number
                                // corresponds to the actual one, it means we are receiving a retransmission,
                                // and our previously sent ACK didn't arrive.
                                // We have received this data already, so we discard if the received number
                                // in the source index is equal to the last saved one.
                                rx_SEQ[ttmsg->getSource()] = 0;
                                // We save the just received message in the index number corresponding to the
                                // source in our rx_SEQ array.
                                //int delay = ttmsg -> getDelivery_delay(); //  Obtain delivery delay.
                                delay_total += (simTime() - ttmsg -> getCt()); // Save it to total amount.
                            }
                        }
                    FSM_Goto(rx_fsm,FROM_0_TO_1); // finally, we move to the transition state between 0 and 1.
                    }
                    else{ // If we have received 6900 messages,
                        FSM_Goto(rx_fsm,FINISH); // We proceed to the finish state.
                    }
                }
            }
            if (ttmsg-> getKind() == 2) FSM_Goto(rx_fsm,SEND_BACK_ACK_IN_0); // If we get data 1, we send back ACK.
            if ((ttmsg-> getKind() == 3) || (msg-> getKind() == 4)) FSM_Goto(rx_fsm, SEND_TX_AT_0);
            // If we get any ACK, we forward to TX and ACK received.
            break;

        case FSM_Enter(WAIT_FOR_DATA_1):
            // Do nothing on entry.
            break;

        case FSM_Exit(WAIT_FOR_DATA_1):
            if (ttmsg-> getKind() == 2){// If we get Data 1
                //We get the delivery delay of the message
                l4Msg *ttmsg = check_and_cast<l4Msg *>(msg); // We cast the copy to our data type.
                if (id_last_1 >= ttmsg -> getId()){
                // If the id is from an old message (smaller Id), we just send ack, no statistics are performed.
                    FSM_Goto(rx_fsm,SEND_BACK_ACK_IN_1);
                } else {
                    id_last_1 = ttmsg-> getId(); // Else, we update the last received Id.
                    if(numRxed != 6900){ // If we have not received 6900 messages
                        if (ind == 21){ // If we're module G+2,
                            if (rx_SEQ[ttmsg->getSource()] != 1){
                                // And if in index of source of data message the last received sequence number
                                // corresponds to the actual one, it means we are receiving a retransmission,
                                // and our previously sent ACK didn't arrive.
                                // We have received this data already, so we discard if the received number
                                // in the source index is equal to the last saved one.
                                rx_SEQ[ttmsg->getSource()] = 1;
                                // We save the just received message in the index number corresponding to the
                                // source in our rx_SEQ array.
                                //int delay = ttmsg -> getDelivery_delay(); //  Obtain delivery delay.
                                delay_total += (simTime() - ttmsg -> getCreationTime()); // Save it to total amount.
                            }
                        }
                        FSM_Goto(rx_fsm,FROM_1_TO_0);  // finally, we move to the transition state between 0 and 1.
                    } else { // If we have received 6900 messages,
                        FSM_Goto(rx_fsm,FINISH); // We move to finish.
                    }
                }
            }
            if (ttmsg-> getKind() == 1) FSM_Goto(rx_fsm,SEND_BACK_ACK_IN_1); // If we get Data 0, we send ack.
            if ((ttmsg-> getKind() == 3) || (ttmsg-> getKind() == 4)) FSM_Goto(rx_fsm, SEND_TX_AT_1);
            // If we get any ack, we forward it to tx.
            break;

        case FSM_Enter(FINISH):
            if (msg -> getKind() == 1 || msg -> getKind() == 2) {
                generateACK(ttmsg); // Generate and send last ack
                delete ttmsg; // Delete last received message.
            }
            break;

        case FSM_Exit(FINISH):
            if ((ttmsg-> getKind() == 3) || (ttmsg-> getKind() == 4)) FSM_Goto(rx_fsm, SEND_TX_AT_FINISH);
            // If we get an ACK on our finish state, we send an ACK received to tx.
            break;

        case FSM_Exit(SEND_TX_AT_0):
            ack_seq = ttmsg->getSEQ(); // Get seq number for ACK received
            ack_id_aux = ttmsg->getId(); // Get Id of message for the ACK received to have same Id.
            delete ttmsg; // Dispose of object.
            if (ack_seq == 0) kind = 5; // If the seq number is 0, set kind 5 (ACK received 0)
            else kind = 6; // If the seq number is 1, set kind 6 (ACK received 1)
            ACK_rxed = new l4Msg("ack_rxed",kind); // Create ack_rxed for tx.
            ACK_rxed -> setSEQ(ack_seq); // Set seq number
            ACK_rxed -> setId(ack_id_aux); // Set id to ACK rxed.
            send(ACK_rxed,"out_to_tx"); // Send to tx
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_0); // Go back to waiting for data 0.
            break;

        case FSM_Exit(SEND_TX_AT_1):
            ack_seq = ttmsg->getSEQ(); // Get seq number for ACK received.
            ack_id_aux = ttmsg->getId(); // Get Id of message for the ACK received to have same Id.
            delete ttmsg; // Dispose of object.
            if (ack_seq == 0) kind = 5;// If the seq number is 0, set kind 5 (ACK received 0)
            else kind = 6; // If the seq number is 1, set kind 6 (ACK received 1)
            ACK_rxed = new l4Msg("ack_rxed",kind); // Create ack_rxed for tx.
            ACK_rxed -> setSEQ(ack_seq); // Set seq numer
            ACK_rxed -> setId(ack_id_aux); // Set id to ACK rxed.
            send(ACK_rxed,"out_to_tx"); // Send to tx
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_1); // Go back to waiting for data 0.
            break;

        case FSM_Exit(FROM_0_TO_1):
            numRxed++; // We succesfully received a data 0 message, we increase number of received.
            generateACK(ttmsg); // We generate and send an ack to the sender.
            delete ttmsg; // We delete the data message.
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_1); // Proceed to wait for data 1 message.
            break;

        case FSM_Exit(SEND_BACK_ACK_IN_0):
            generateACK(ttmsg); // We generate ack for data 1 message.
            delete ttmsg; // We delete data message.
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_0); // Go back to waiting for 0.
            break;

        case FSM_Exit(FROM_1_TO_0):
            numRxed++; // We succesfully received a data 0 message, we increase number of received.
            generateACK(ttmsg); // We generate and send an ack to the sender.
            delete ttmsg; // We delete the data message.
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_0); // Proceed to wait for data 1 message.
            break;

        case FSM_Exit(SEND_BACK_ACK_IN_1):
            generateACK(ttmsg); // We generate ack for data 1 message.
            delete ttmsg; // We delete data message.
            FSM_Goto(rx_fsm,WAIT_FOR_DATA_1); // Go back to waiting for 0.
            break;

        case FSM_Exit(SEND_TX_AT_FINISH):
            ack_seq = ttmsg->getSEQ(); // Get seq number for ACK
            ack_id_aux = ttmsg->getId(); // Get Id of message for the ACK received to have same Id.
            delete ttmsg; // Dispose of object.
            if (ack_seq == 0) kind = 5; // If the seq number is 0, set kind 5 (ACK received 0)
            else kind = 6; // If the seq number is 1, set kind 6 (ACK received 1)
            ACK_rxed = new l4Msg("ack_rxed",kind); // Create ack_rxed for tx.
            ACK_rxed -> setSEQ(ack_seq); // Set seq number
            ACK_rxed -> setId(ack_id_aux); // Send to tx
            send(ACK_rxed,"out_to_tx"); // Send to tx
            FSM_Goto(rx_fsm,FINISH); // Go back to waiting for data 0.
            break;
    }
}

void l4_rx::generateACK(l4Msg *msg)
{
    // Generate and send ACK
    gen_ack++; // Increase number of generated ACKs.
    int ack_seq = msg -> getSEQ(); // Get sequence number
    int ack_dest = msg -> getSource(); // Get source
    int ack_id = msg -> getId(); // Get Id from data message.
    if (ack_seq == 0) kind = 3; // If seq is 0, it's an ACK 0, of kind 3
    else kind = 4; // If seq is 1, it's an ACK 1, of kind 4.
    char ackname[25];
    sprintf(ackname, "ACK-from-%d-to-%d-#%d", ind, ack_dest, ack_seq);
    l4Msg *ack = new l4Msg(ackname,kind); // Create ack message
    // Add fields to it.
    ack->setSEQ(ack_seq);
    ack->setId(ack_id);
    ack->setSource(ind);
    ack->setDestination(ack_dest);
    ack->setDestination_row((int)ack_dest/5);
    ack->setDestination_column(ack_dest%5);
    send(ack,"out_to_link"); // Send out to link.
}

void l4_rx::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in the console.
   recordScalar("Generated ACKs", gen_ack); // Save generated ACKs.
    if (ind == 21){ // Only performed in module 21.
    Module21_Avg = delay_total/(simtime_t)numRxed; // Calculate delivery delay
    recordScalar("Mod21Avg", Module21_Avg); // Save data
    EV << "Total delay is " << delay_total << " and I received " << numRxed << endl;
    }
}
