#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l2t2a_m.h" // Message data type

using namespace omnetpp;

class l2t2a_tx : public cSimpleModule
{
  private:
    simtime_t neg_exp_time;  // Variable in which we will store our neg_exp_time.
    cMessage *timeoutEvent;  // Message schedule in order to allow for retransmissions after a certain timeout.
    cMessage *newMsg; // Message scheduled in order to generate a new data message.
    int seq, ind;  // Message sequence number and module index.
    l2t2aMsg *message;  // Message that has to be re-sent in case of time out.

  public:
    l2t2a_tx(); // Constructor
    virtual ~l2t2a_tx(); // Destructor

  protected:
    virtual l2t2aMsg *generateNewMessage();
    //Function to generate new message.
    virtual void sendCopyOf(cMessage *msg);
    // Function to send copy of last generated message.
    virtual void initialize() override;
    // Function to initialize the tx module.
    virtual void handleMessage(cMessage *msg) override;
    // Function that tells the module what to do when receiving a message.
};

Define_Module(l2t2a_tx); //Registering module.

l2t2a_tx::l2t2a_tx()
{
    timeoutEvent = newMsg = nullptr; // At the moment the created messages point to nothing.
    message = nullptr;
}


l2t2a_tx::~l2t2a_tx()
{
    cancelAndDelete(timeoutEvent); // Cancel timeoutEvent if scheduled, at module destruction. Also delete object.
    cancelAndDelete(newMsg); // Cancel the self scheduled message for data message generation at module destruction. Also delete object.
}

void l2t2a_tx::initialize()
{
    // Initialize variables.
    seq = 0; // Initial sequence number.
    WATCH(seq); // Useful debugging tool.
    neg_exp_time = par("delayTime"); //Obtain the value sample of the negative exponential random variable declared in .ini file.
    timeoutEvent = new cMessage("timeoutEvent"); // Create a new timeout event message.
    newMsg = new cMessage("newMsg"); // Create a new self message for the module to generate and send a new message.
    ind = getParentModule() -> getIndex(); // Get index of the current module.
    // Generate and send initial message.
    scheduleAt(simTime()+neg_exp_time, newMsg);
    // Schedule self message in order to generate and transmit data message after a negative exponential time.
}

void l2t2a_tx::handleMessage(cMessage *msg)
{
    if (msg == timeoutEvent) {
        // If we receive the timeout event, that means the packet hasn't
        // arrived in time and we have to re-send it.
        message -> setDelivery_delay(message->getDelivery_delay()+500);
        // Add the timeout time (in ms) to the initial delay of the message.
        sendCopyOf(message); // Re send copy of message.
        scheduleAt(simTime()+0.5, timeoutEvent); // Re schedule timeout for the same amount.
    }
    else if (msg == newMsg) { // When we receive the self message "newMsg" after a negative exponential time,
        message = generateNewMessage(); // We generate a new message
        sendCopyOf(message); // and we send a  copy of it.
        scheduleAt(simTime()+0.5, timeoutEvent); // We also schedule a timeout.
        // The value of 0.5 is used because the worst case scenario for a message is to perform 8 hops, out of those
        // 8 hops, only two can be performed over the delayed connections to node G = 19. For the worst case, with
        // parameter L = 90, we have 180+60 = 240 ms, times 2 to account for the round trip time, its 480 ms.
        // It is then rounded to 500 ms.
    } else {// The final case is receiving an acknowledgment.
        if (msg->getKind() == 2) { // Kind 2 is ACK received kind.
            delete msg; // We don't need any information from the message, so we delete it.
            // However, what is really important is to cancel the timeout and prepare to send a new message.
            cancelEvent(timeoutEvent);
            // We generate a newMsg to self only if there's not any other newMsg scheduled
            // and we have not sent 19000 already.
            if (seq != 19001 && !(newMsg->isScheduled())){
                delete message; // We delete the copy we had of the previous data message.
                neg_exp_time = par("delayTime"); // We obtain the negative exponential time for the newMsg self message.
                scheduleAt(simTime()+neg_exp_time, newMsg);// We schedule the new message.
            } else {
                if (!(newMsg->isScheduled())) delete message;
                // If we have already sent 19000 messages and we don't have a newMsg scheduled,
                // we delete the copy of the message WHICH IS THE LAST MESSAGE SENT.
            }
        }
    }
}

l2t2aMsg *l2t2a_tx::generateNewMessage()
{
    // Generate a message with a different name every time.
    char msgname[20];
    int size  = getParentModule() -> getVectorSize(); // Amount of modules in the network.
    int dest = intuniform(0,size-2); // We find the destination at random.
    if (dest >= ind) dest++;  // Explained before in lab 2 task 1 files.
    int dest_row = (int) dest/5; // Obtaining destination row
    int dest_col = (int) dest%5; // Obtaining destination column
    sprintf(msgname, "Msg from %d to %d - SEQ %d", ind, dest, seq); // Put name on string msgname.
    l2t2aMsg *msg = new l2t2aMsg(msgname); // Create a new data message and give it msg name.
    msg-> setDestination_column(dest_col); // Assign destination column,
    msg-> setDestination_row(dest_row); // destination row,
    msg -> setSource(ind); // source
    msg -> setSeq(++seq); // and sequence number.
    // We start from 1 because in rx we use an array to save the seq number of the last received message from each
    // module, and this array is initialized with 0. So, we start from 1 to prevent a mismatch.
    return msg; // Give message back to caller.
}

void l2t2a_tx::sendCopyOf(cMessage *msg)
{
    // Duplicate message and send the copy.
    cMessage *copy = (cMessage *)msg->dup();
    send(copy, "out");
}
