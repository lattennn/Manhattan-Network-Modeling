#define FSM_DEBUG // Debugging
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l3Msg_m.h"


using namespace omnetpp;

#define G 19 // Group number.

class l3_tx : public cSimpleModule
{
  private:
    // configuration
    int numTxed, dest, ind; // Number of sent messages, destination of messages and module index.
    simtime_t neg_exp_time; // Parameter for negative exponential time
    cMessage *timeoutEvent; // Selfmessage for timeout
    cMessage *newMsg; // Selfmessage to create new messages
    l3Msg *message; // Last message sent.

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
    l3_tx();
    virtual ~l3_tx();

  protected:
    virtual void initialize() override;
    // Initialization function
    virtual void handleMessage(cMessage *msg) override;
    // We decide what to do with the messages.
    virtual l3Msg *generateMessage(int seq);
    // We generate new messages.
    virtual void sendCopyOf(cMessage *msg);
    // We send copy of messages.

};

Define_Module(l3_tx);


l3_tx::l3_tx()
{
    timeoutEvent = newMsg = nullptr; // At the moment the created messages point to nothing.
    message = nullptr;
}

l3_tx::~l3_tx()
{
    cancelAndDelete(timeoutEvent); // Cancel timeoutEvent if scheduled, at module destruction. Also delete object.
    cancelAndDelete(newMsg); // Cancel the self scheduled message for data message generation at module destruction. Also delete object.
    delete message; // Delete last message saved.
}

void l3_tx::initialize()
{
    numTxed = 0;
    WATCH(numTxed); // Debugging
    tx_fsm.setName("tx_fsm");
    neg_exp_time = par("delayTime"); //Obtain the value sample of the negative exponential random variable declared in .ini file.
    timeoutEvent = new cMessage("timeoutEvent"); // Create a new timeout event message.
    newMsg = new cMessage("newMsg"); // Create a new self message for the module to generate and send a new message.
    ind = getParentModule() -> getIndex(); // Get index of the current module.
    // Generate and send initial message.
    scheduleAt(simTime()+neg_exp_time, newMsg);
    // Schedule self message in order to generate and transmit data message after a negative exponential time.
    dest = (ind + G)%25; // Destination is fixed.
}

void l3_tx::handleMessage(cMessage *msg)
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
                message = generateMessage(0); // We generate a new message
                sendCopyOf(message); // and we send a  copy of it.
                scheduleAt(simTime()+0.5, timeoutEvent); // We also schedule a timeout.
                // The value of 0.5 is used because the worst case scenario for a message is to perform 8 hops, out of those
                // 8 hops, only two can be performed over the delayed connections to node G = 19. For the worst case, with
                // parameter L = 90, we have 180+60 = 240 ms, times 2 to account for the round trip time, its 480 ms.
                // It is then 'rounded to 500 ms.
            }
            if (msg == timeoutEvent) { // If we enter from timeoutEvent,
                // That means the packet hasn't arrived in time and we have to re-send it.
                message -> setDelivery_delay(message->getDelivery_delay()+500);
                // Add the timeout time (in ms) to the initial delay of the message.
                sendCopyOf(message); // Re send copy of message.
                scheduleAt(simTime()+0.5, timeoutEvent); // Re schedule timeout for the same amount.
            }
            if (msg -> getKind() == 6){ // If we enter from "wait for ack 1".
                delete msg; // We don't need any information from the message, so we delete it.
                // However, what is really important is to cancel the timeout and prepare to send a new message.
                cancelEvent(timeoutEvent);
                delete message; // We delete the copy we had of the previous data message.
                neg_exp_time = par("delayTime"); // We obtain the negative exponential time for the newMsg self message.
                scheduleAt(simTime()+neg_exp_time, newMsg);// We schedule to create a new message.
            }
            break;

        case FSM_Exit(WAIT_FOR_ACK_0):
            if ((msg == timeoutEvent)||(msg == newMsg)) FSM_Goto(tx_fsm,WAIT_FOR_ACK_0);
            // If we receive a timetoutEvent or newMsg, reenter.
            if (msg-> getKind() == 6 ) FSM_Goto(tx_fsm,DISCARD_ACK_IN_0);
            // if we get ACK 1, we discard.
            if (msg-> getKind() == 5){ // If we get ack 0, we increase the number of sent messages
                numTxed++;
                if (numTxed != 19000) FSM_Goto(tx_fsm,WAIT_FOR_ACK_1);
                // And if we have not sent 19000, we go to wait for ACK 1
                else{ // If we have sent 19000,
                    delete message; // We delete last copy,
                    delete msg; // last message received
                    FSM_Goto(tx_fsm,FINISH); // and go to finish state.
                }
            }
            break;

        case FSM_Enter(WAIT_FOR_ACK_1):
            if (msg == newMsg){ // If we enter from newMsg
                message = generateMessage(1); // We generate a new message
                sendCopyOf(message); // and we send a  copy of it.
                scheduleAt(simTime()+2, timeoutEvent); // We also schedule a timeout.
                // The value of 2 is used to account for the possibilites of multiple passages
                // through long channel before the hopcount reaches 30.
            }
            if (msg == timeoutEvent) { // If we enter from timeout event,
                // That means the packet hasn't arrived in time and we have to re-send it.
                message -> setDelivery_delay(message->getDelivery_delay()+2000);
                // Add the timeout time (in ms) to the initial delay of the message.
                sendCopyOf(message); // Re send copy of message.
                scheduleAt(simTime()+2, timeoutEvent); // Re schedule timeout for the same amount.
            }
            if (msg -> getKind() == 5){ // We come from "wait for ack 0".
                delete msg; // We don't need any information from the message, so we delete it.
                // However, what is really important is to cancel the timeout and prepare to send a new message.
                cancelEvent(timeoutEvent);
                delete message; // We delete the copy we had of the previous data message.
                neg_exp_time = par("delayTime"); // We obtain the negative exponential time for the newMsg self message.
                scheduleAt(simTime()+neg_exp_time, newMsg);// We schedule the new message.
            }
            break;

        case FSM_Exit(WAIT_FOR_ACK_1):
            if ((msg == timeoutEvent)||(msg == newMsg)) FSM_Goto(tx_fsm,WAIT_FOR_ACK_1);
            // If we receive timeoutEvent or newMsg, we reenter.
            if (msg-> getKind() == 5 ) FSM_Goto(tx_fsm,DISCARD_ACK_IN_1);
            // If we get ACK 0, discard
            if (msg-> getKind() == 6){ // If we get ACK 1,
                numTxed++; // increase number of sent messages.
                // If we have not already transmitted 19000 messages, we go to wait for ACK 0
                if (numTxed != 19000) FSM_Goto(tx_fsm,WAIT_FOR_ACK_0);
                else { // If we have sent 19000, we delete last received message and move to finish.
                    delete msg;
                    FSM_Goto(tx_fsm,FINISH);
                }
            }
            break;

        case FSM_Enter(FINISH):
            finish(); // We invoke finish function.
            break;

        case FSM_Exit(FINISH):
            break;

        case FSM_Exit(DISCARD_ACK_IN_0):
            delete msg; // We simply delete the message
            FSM_Goto(tx_fsm,WAIT_FOR_ACK_0); // and go back
            break;

        case FSM_Exit(DISCARD_ACK_IN_1):
            delete msg;// We simply delete the message
            FSM_Goto(tx_fsm,WAIT_FOR_ACK_1); // and go back
            break;
    }
}


l3Msg *l3_tx::generateMessage(int seq)
{
    // We generate a new message.
    char ackname[25];
    sprintf(ackname, "DataMsg-from-%d-to-%d-#%d", ind, dest, seq);
    l3Msg *message = new l3Msg(ackname); // We create new message
    // and add all the fields.
    message->setSEQ(seq);
    message->setKind(seq+1); // Data 0 -> kind 1, data 1 -> kind 2.
    message->setSource(ind);
    message->setDestination(dest);
    message->setDestination_row((int)dest/5);
    message->setDestination_column(dest%5);
    return message; // Give message back to invoker.
}

void l3_tx::sendCopyOf(cMessage *msg)
{
    // Duplicate message and send the copy.
    cMessage *copy = (cMessage *)msg->dup();
    send(copy, "out");
}

