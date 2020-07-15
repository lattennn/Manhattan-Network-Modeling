#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l1t1aMsg_m.h"

using namespace omnetpp;


class l1t1a : public cSimpleModule
{
  private:
    long numSent; //Number of sent packages from the module object.
    cHistogram hopCountStats; //OMNeT++ classes for statistical analysis.
    cOutVector hopCountVector;

  protected:
    virtual l1t1aMsg *generateMessage();
    //Function for message generation.
    virtual void forwardMessage(l1t1aMsg *msg);
    //Function for message forwarding on output gates.
    virtual void initialize() override;
    //Function for initialization of modules.
    virtual void handleMessage(cMessage *msg) override;
    //Function in which we decide what to do given a message.
    virtual void finish() override;
    //Function to conclude the simulation.
};

Define_Module(l1t1a); //Module registration

void l1t1a::initialize()
{
    // Variable initialization.
    numSent = 0;
    WATCH(numSent);
    //WATCH makes the variable visible in user interface.

    // Statistical analysis classes provided by OMNeT++.
    hopCountStats.setName("hopCountStats");
    hopCountVector.setName("HopCount");

    // Module 0 generates and sends the first message.
    if (getIndex() == 0) {
        // Start the process by scheduling a self message at time 0 so statistics can be adequately
        // collected.
        l1t1aMsg *msg = generateMessage();
        scheduleAt(0.0, msg);
        numSent++; //Increase the number of messages sent by 0.
    }
}

void l1t1a::handleMessage(cMessage *msg)
{
    l1t1aMsg *ttmsg = check_and_cast<l1t1aMsg *>(msg);
    // Check that the received message belongs to class l1t1aMsg, our predefined message class.

    if (ttmsg->getDestination() == getIndex()) {
        // Message arrived to destination.
        int hopcount = ttmsg->getHopCount();
        // Get the number of hops.
        EV << "Message " << ttmsg << " arrived after " << hopcount << " hops.\n";
        bubble("Transmission successful. Creating a new one!"); // Cosmetic modification.

        // Saving statistics.
        hopCountVector.record(hopcount);
        hopCountStats.collect(hopcount);
        delete ttmsg; // We make sure of deleting the already used object.

        if (numSent != 19000){
        // Generate another one ONLY if we have not sent 19000 messages on this module.
        EV << "Generating another message: ";
        l1t1aMsg *newmsg = generateMessage(); // Generation.
        EV << newmsg << endl;
        numSent++; // Module statistic update.
        forwardMessage(newmsg); // Forwarding
        }
    }
    else {
        // If we are not the destination, we need to forward the message.
        forwardMessage(ttmsg);
    }
}

l1t1aMsg *l1t1a::generateMessage()
{
    // Produce source and destination addresses.
    int src = getIndex();
    int n = getVectorSize(); // How many modules are there in the network?
    int dest = intuniform(0, n-2); // Integer uniformly distributed number between 0 and n-2.
    // n modules in the network
    // - 1 (accounting for indexes as they go from 0 to n-1)
    // - 1 (we can't send to self)

    if (dest >= src)
        dest++;
    // dest will be between 0 and n-2. What happens for module #n-1?
    // If dest is lower than the actual index, we have no problem.
    //
    // However, if dest is equal to actual index, we have to change the number because
    // we can't send to self. We add 1, as subtracting 1 would change the uniformity of
    // the chosen number. Both index-1 and index values of dest will converge in the index-1th
    // module.
    //
    // If dest is greater than the actual index, we also need to add 1, as this would solve
    // the n-1th module issue. Also, the index+1-th module would not be more probable then the
    // rest of the modules because of the +1 addition performed in case of obtaining dest
    // equal to actual index: index and index+1 values of dest would converge in the
    // index+1th module.

    char msgname[20]; // Message name.
    sprintf(msgname, "Msg-from-%d-to-%d", src, dest); // Save message name on message name variable.

    // Create message object and set source and destination field.
    l1t1aMsg *msg = new l1t1aMsg(msgname);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg; // Return object to calling module.
}

void l1t1a::forwardMessage(l1t1aMsg *msg)
{
    // Increment hop count BEFORE SENDING.
    msg->setHopCount(msg->getHopCount()+1);

    // We choose at output gate at random.
    int n = gateSize("out");
    int k = intuniform(0, n-1);

    EV << "Forwarding message " << msg << " on gate[" << k << "]\n";
    send(msg, "out", k); // Send message object through gate out[k].
}

void l1t1a::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in the console.
    EV << "Sent:  " << numSent << endl;
    EV << "Hop count, min:    " << hopCountStats.getMin() << endl;
    EV << "Hop count, max:    " << hopCountStats.getMax() << endl;
    EV << "Hop count, mean:   " << hopCountStats.getMean() << endl;
    EV << "Hop count, stddev: " << hopCountStats.getStddev() << endl;

    recordScalar("#sent", numSent);

    hopCountStats.recordAs("hop count");
}
