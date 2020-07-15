#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l1t2Msg_m.h"

using namespace omnetpp;

class l1t2 : public cSimpleModule
{
  private:
    long numSent; // Number of sent packages from the module object.
    double channel_delay; // Delay variable for average delay calculation.
    cHistogram delayStats;
    cOutVector delayVector;

  protected:
    virtual l1t2Msg *generateMessage();
    //Function for message generation.
    virtual void forwardMessage(l1t2Msg *msg);
    //Function for message forwarding on output gates.
    virtual void initialize() override;
    //Function for initialization of modules.
    virtual void handleMessage(cMessage *msg) override;
    //Function in which we decide what to do given a message.

    virtual void finish() override;
    //Function to conclude the simulation.

};

Define_Module(l1t2); //Module registration

void l1t2::initialize()
{
    // Variable initialization.
    numSent = 0;
    WATCH(numSent);
    channel_delay = par("L"); // Channel delay parameter taken from ini file.

    // Statistical analysis classes provided by OMNeT++.
    delayStats.setName("delDelStats");
    delayVector.setName("delivery_delay");

    // Module 0 generates and sends the first message.
    if (getIndex() == 0) {
        // Start the process by scheduling a self message at time 0 so statistics can be adequately
        // collected.
        l1t2Msg *msg = generateMessage();
        scheduleAt(0.0, msg); //Increase the number of messages sent by 0.
    }
}

void l1t2::handleMessage(cMessage *msg)
{
    l1t2Msg *ttmsg = check_and_cast<l1t2Msg *>(msg);
    // Check that the received message belongs to class l1t2Msg, our predefined message class.

    if (ttmsg->getDestination() == getIndex()) {
        int delivery_delay = ttmsg->getDelivery_delay();
        // Message arrived to destination.
        if (getIndex() == 21){ // We're interested in saving packet delivery delay on module 21.
            // Saving statistics.
            delayVector.record(delivery_delay);
            delayStats.collect(delivery_delay);
        }
        EV << "Message " << ttmsg << " arrived after " << delivery_delay << " ms.\n";
        bubble("Transmission successful. Creating a new one!"); // Cosmetic modification.
        delete ttmsg; // We make sure of deleting the already used object.

        if (numSent != 19000){
        // Generate another one ONLY if we have not sent 19000 messages on this module.
        EV << "Generating another message: ";
        l1t2Msg *newmsg = generateMessage(); // Generation.
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

l1t2Msg *l1t2::generateMessage()
{
    // Produce source and destination addresses.
    int src = getIndex();
    int n = getVectorSize(); // How many modules are there in the network?
    int dest = intuniform(0, n-2);// Integer uniformly distributed number between 0 and n-2.
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
    sprintf(msgname, "Msg-from-%d-to-%d", src, dest);
    // Save message name on message name variable.

    // Create message object and set source and destination field.
    l1t2Msg *msg = new l1t2Msg(msgname);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg; // Return object to calling module.
}

void l1t2::forwardMessage(l1t2Msg *msg)
{
    int n = gateSize("out");
    int k = intuniform(0, n-1);
    int ind = getIndex(); // We get the index on the actual module.
    EV << "Forwarding message " << msg << " on gate[" << k << "]\n";
    // When k is 0, horizontal output gate.
    // When k is 1, vertical output gate.

    if ((ind == 19) || (ind == 14 && k == 1) || (ind == 18 && k == 0))
    // If we send in any channel out-going or in-going module 19, add delay L ms.
    {
        msg->setDelivery_delay(msg->getDelivery_delay()+channel_delay); // Increase package arrival time.
    } else {
    // Else, add 10 ms.
        msg->setDelivery_delay(msg->getDelivery_delay()+10); // Increase package arrival time.
    }

    send(msg, "out", k);
}

void l1t2::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in the console.
    EV << "Sent:     " << numSent << endl;
    EV << "Delivery delay, min:    " << delayStats.getMin() << endl;
    EV << "Delivery delay, max:    " << delayStats.getMax() << endl;
    EV << "Delivery delay, mean:   " << delayStats.getMean() << endl; // Average packet delay.
    EV << "Delivery delay, stddev: " << delayStats.getStddev() << endl;

    recordScalar("#sent", numSent);

    delayStats.recordAs("Delivery delay");
}
