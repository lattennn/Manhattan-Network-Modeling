#define FSM_DEBUG // Debugging
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l4Msg_m.h"


using namespace omnetpp;

#define G 19 // Group number.

class l4_tx : public cSimpleModule
{
  private:
    int numTxed, dest, ind; // Number of sent messages, destination of messages and module index.
    simtime_t neg_exp_time; // Parameter for negative exponential time
    cMessage *timeoutEvent; // Selfmessage for timeout
    cMessage *newMsg; // Selfmessage to create new messages
    l4Msg *message; // Last message sent.
    simtime_t timeout; // Timeout for retransmissions.
    simtime_t ct_last; // Creation time of last message sent, kept for future retransmissions.
    int id_lastmsg_0, id_lastmsg_1; //Id of last messages of sequence number 0 and 1, respectively
    double gen_msg; // Number of generated messages.

    cFSM tx_fsm; // FSM name
    enum { // States
        INIT = 0,
        WAIT_FOR_ACK_0 = FSM_Steady(1), // Waiting for ack 0
        WAIT_FOR_ACK_1 = FSM_Steady(2), // Waiting for ack 1
        FINISH = FSM_Steady(3),
        DISCARD_ACK_IN_0 = FSM_Transient(1), // When waiting for ack 0, receiving ack 1.
        DISCARD_ACK_IN_1 = FSM_Transient(2), // When waiting for ack 1, receiving ack 0.
    };

  public:
    l4_tx();
    virtual ~l4_tx();

  protected:
    virtual void initialize() override;
    // Initialization function
    virtual void handleMessage(cMessage *msg) override;
    // We decide what to do with the messages.
    virtual l4Msg *generateMessage(int seq);
    // We generate new messages.
    virtual void finish() override;
    // Called at the end of the simulation.
};

Define_Module(l4_tx);


l4_tx::l4_tx()
{
    timeoutEvent = newMsg = nullptr; // At the moment the created messages point to nothing.
    message = nullptr;
}

l4_tx::~l4_tx()
{
    cancelAndDelete(timeoutEvent); // Cancel timeoutEvent if scheduled, at module destruction. Also delete object.
    cancelAndDelete(newMsg); // Cancel the self scheduled message for data message generation at module destruction. Also delete object.
}

void l4_tx::initialize()
{
    // Variable initialization to 0.
    numTxed = 0;
    WATCH(numTxed); // Debugging
    gen_msg = 0;
    id_lastmsg_0 = id_lastmsg_1 = 0;
    tx_fsm.setName("tx_fsm");
    neg_exp_time = par("delayTime"); //Obtain the value sample of the negative exponential random variable declared in .ini file.
    timeoutEvent = new cMessage("timeoutEvent"); // Create a new timeout event message.
    newMsg = new cMessage("newMsg"); // Create a new self message for the module to generate and send a new message.
    ind = getParentModule() -> getIndex(); // Get index of the current module.
    // Generate and send initial message.
    scheduleAt(simTime()+neg_exp_time, newMsg);
    // Schedule self message in order to generate and transmit data message after a negative exponential time.
    dest = (ind + G)%25; // Destination is fixed.
    timeout = 0.3; // Set timeout to 0.3 seconds = 300 ms.
}

void l4_tx::handleMessage(cMessage *msg)
{
    // Kind 1 = Data 0
    // Kind 2 = Data 1
    // Kind 3 = Ack 0
    // Kind 4 = Ack 1
    // Kind 5 = Ack_rxed 0
    // Kind 6 = Ack_rxed 1

    FSM_Switch(tx_fsm) {
        case FSM_Exit(INIT):
            FSM_Goto(tx_fsm, WAIT_FOR_ACK_0); //Start at waiting for ACK 0.
            break;

        case FSM_Enter(WAIT_FOR_ACK_0):
            if (msg == newMsg){ // If we enter from newMsg,
                ct_last = simTime(); // Record current simulation time for the retransmissions of this message.
                id_lastmsg_0 += 1; // Increase the id by 1 (new message).
                message = generateMessage(0); // We generate a new message with sequence number 0
                message -> setCt(ct_last); // We set the creation time in the message,
                message -> setId(id_lastmsg_0); // And the id.
                send(message, "out"); // We send it towards link layer.
                if(!timeoutEvent -> isScheduled()) scheduleAt(simTime()+timeout, timeoutEvent); // We also schedule a timeout.
            }
            if (msg == timeoutEvent) { // If we enter from timeoutEvent,
                // That means the packet hasn't arrived in time and we have to re-send it.
                message = generateMessage(0); // We generate a new message with sequence number 0.
                message -> setCt(ct_last); // We set the last creation time.
                message -> setId(id_lastmsg_0); // And the last id.
                send(message, "out"); // We send towards link layer.
                if(!timeoutEvent -> isScheduled()) scheduleAt(simTime()+timeout, timeoutEvent); // Re schedule timeout for the same amount.
            }
            if (msg -> getKind() == 6){ // If we enter from "wait for ack 1".
                delete msg; // We don't need any information from the message, so we delete it.
                // However, what is really important is to cancel the timeout and prepare to send a new message.
                cancelEvent(timeoutEvent);
                neg_exp_time = par("delayTime"); // We obtain the negative exponential time for the newMsg self message.
                if(!newMsg -> isScheduled()) scheduleAt(simTime()+neg_exp_time, newMsg);// We schedule to create a new message.
            }
            break;

        case FSM_Exit(WAIT_FOR_ACK_0):
            if ((msg == timeoutEvent)||(msg == newMsg)) FSM_Goto(tx_fsm,WAIT_FOR_ACK_0);
            // If we receive a timetoutEvent or newMsg, reenter.
            if (msg-> getKind() == 6 ) FSM_Goto(tx_fsm,DISCARD_ACK_IN_0);
            // if we get ACK 1, we discard.
            if (msg-> getKind() == 5){ // If we get ack 0, we increase the number of sent messages
                cMessage *copy = (cMessage *)msg->dup(); // We create a copy
                l4Msg *ttmsg = check_and_cast<l4Msg *>(copy); // We cast the copy to our data type.
                if (id_lastmsg_0 > ttmsg -> getId()){ // If the id of the ACK received is from an old message, we discard it and go back
                    delete ttmsg;
                    delete msg;
                    FSM_Goto(tx_fsm,WAIT_FOR_ACK_0);
                } else { // Else, we increase te number of succesfully transmitted messages,
                    numTxed++;
                    if (numTxed != 6900){ // If we have not sent 6900, we wait for ACK 1
                        delete ttmsg;
                        FSM_Goto(tx_fsm,WAIT_FOR_ACK_1);
                    }
                    else{ // If we have sent 6900,
                        delete ttmsg; // We delete last copy,
                        delete msg; // last message received
                        FSM_Goto(tx_fsm,FINISH); // and go to finish state.
                    }
                }
            }
            break;

        case FSM_Enter(WAIT_FOR_ACK_1): //Similar to wait for ACK 0, but with variables corresponding to 1.
            if (msg == newMsg){ // If we enter from newMsg
                id_lastmsg_1 += 1; // Increase id for seq 1 messages.
                ct_last = simTime(); // Get new last creation time.
                message = generateMessage(1); // We generate a new message with seq 1
                message -> setCt(ct_last); // And add corresponding fields.
                message -> setId(id_lastmsg_1);
                send(message, "out"); // Send to link layer.
                if(!timeoutEvent -> isScheduled()) scheduleAt(simTime()+timeout, timeoutEvent); // We also schedule a timeout.
            }
            if (msg == timeoutEvent) { // If we enter from timeout event,
                // That means the packet hasn't arrived in time and we have to re-send it.
                message = generateMessage(1); // We generate a new message
                message -> setCt(ct_last); // Add stored fields.
                message -> setId(id_lastmsg_1);
                send(message, "out"); // send to link layer.
                if(!timeoutEvent -> isScheduled()) scheduleAt(simTime()+timeout, timeoutEvent); // Re schedule timeout for the same amount.
            }
            if (msg -> getKind() == 5){ // We come from "wait for ack 0".
                delete msg; // We don't need any information from the message, so we delete it.
                // However, what is really important is to cancel the timeout and prepare to send a new message.
                cancelEvent(timeoutEvent);
                neg_exp_time = par("delayTime"); // We obtain the negative exponential time for the newMsg self message.
                if(!newMsg -> isScheduled()) scheduleAt(simTime()+neg_exp_time, newMsg);// We schedule the new message.
            }
            break;

        case FSM_Exit(WAIT_FOR_ACK_1):
            if ((msg == timeoutEvent)||(msg == newMsg)) FSM_Goto(tx_fsm,WAIT_FOR_ACK_1);
            // If we receive timeoutEvent or newMsg, we reenter.
            if (msg-> getKind() == 5 ) FSM_Goto(tx_fsm,DISCARD_ACK_IN_1);
            // If we get ACK 0, discard
            if (msg-> getKind() == 6){ // If we get ACK 1,
                cMessage *copy = (cMessage *)msg->dup(); // We create a copy
                l4Msg *ttmsg = check_and_cast<l4Msg *>(copy); // We cast the copy to our data type.
                if (id_lastmsg_1 > ttmsg -> getId()){ // If message is an old message, discard.
                    delete ttmsg;
                    delete msg;
                    FSM_Goto(tx_fsm,WAIT_FOR_ACK_1);
                } else { // Else, increase succesful transmissions and continue to next step.
                    numTxed++; // increase number of sent messages.
                    // If we have not already transmitted 5900 messages, we go to wait for ACK 0
                    if (numTxed != 6900){
                        delete ttmsg;
                        FSM_Goto(tx_fsm,WAIT_FOR_ACK_0);
                    }
                    else { // If we have sent 6900, we delete last received message and move to finish.
                        delete ttmsg;
                        delete msg;
                        FSM_Goto(tx_fsm,FINISH);
                    }
                }
            }
            break;

        case FSM_Enter(FINISH):
            //finish(); // We invoke finish function.
            break;

        case FSM_Exit(FINISH):
            break;

        case FSM_Exit(DISCARD_ACK_IN_0):
            //delete msg; // We simply delete the message
            FSM_Goto(tx_fsm,WAIT_FOR_ACK_0); // and go back
            break;

        case FSM_Exit(DISCARD_ACK_IN_1):
            //delete msg;// We simply delete the message
            FSM_Goto(tx_fsm,WAIT_FOR_ACK_1); // and go back
            break;
    }
}


l4Msg *l4_tx::generateMessage(int seq)
{
    // We generate a new message.
    gen_msg++; // Increase the number of generated messages.
    char ackname[25];
    sprintf(ackname, "DataMsg-from-%d-to-%d-#%d", ind, dest, seq);
    l4Msg *message = new l4Msg(ackname); // We create new message
    // and add all the fields.
    message->setSEQ(seq);
    message->setKind(seq+1); // Data 0 -> kind 1, data 1 -> kind 2.
    message->setSource(ind);
    message->setDestination(dest);
    message->setDestination_row((int)dest/5);
    message->setDestination_column(dest%5);
    return message; // Give message back to invoker.
}

void l4_tx::finish()
{
    recordScalar("Generated messages", gen_msg); // Ssve number of generated messages.
}
