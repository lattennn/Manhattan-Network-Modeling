#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l1t3Msg_m.h"

using namespace omnetpp;

class l1t3 : public cSimpleModule
{
  private:
    long numSent; // Number of sent packages from the module object.
    double channel_delay; // Delay variable for average delay calculation.
    int ind, row, col;
    cHistogram delayStats;
    cOutVector delayVector;

  protected:
    virtual l1t3Msg *generateMessage();
    //Function for message generation.
    virtual void forwardMessageHorizontally(l1t3Msg *msg);
    //Function for message forwarding on horizontal ring output gate.
    virtual void forwardMessageVertically(l1t3Msg *msg);
    //Function for message forwarding on vertical ring output gate.
    virtual void initialize() override;
    //Function for initialization of modules.
    virtual void handleMessage(cMessage *msg) override;
    //Function in which we decide what to do given a message.

    virtual void finish() override;
    //Function to conclude the simulation.
};

Define_Module(l1t3); //Module registration

void l1t3::initialize()
{
    // Variable initialization.
    numSent = 0;
    WATCH(numSent);
    channel_delay = par("L");  // Channel delay parameter taken from ini file.
    ind = getIndex();
    row = (int) ind/5; // i, starting from 0.
    col = ind%5; // j, starting from 0.

    // Statistical analysis classes provided by OMNeT++.
    delayStats.setName("delDelStats");
    delayVector.setName("delivery_delay");

    // Module 0 generates and sends the first message.
    if (getIndex() == 0) {
        // Start the process by scheduling a self message at time 0 so statistics can be adequately
        // collected.
        l1t3Msg *msg = generateMessage();
        scheduleAt(0.0, msg);
    }
}

void l1t3::handleMessage(cMessage *msg)
{
    l1t3Msg *ttmsg = check_and_cast<l1t3Msg *>(msg);
    // Check that the received message belongs to class l1t3Msg, our predefined message class.

    if (ttmsg->getDestination_column() == col) { //If we are in the destination column,
        if (ttmsg->getDestination_row() == row){ // And in the destination row
            // It means we've reached the destination.
            int delivery_delay = ttmsg->getDelivery_delay();
            if (ind == 21){ // We're interested in saving packet delivery delay on module 21.
                // We obtain the desired value and proceed to record it.
                delayVector.record(delivery_delay);
                delayStats.collect(delivery_delay);
            }
            EV << "Message " << ttmsg << " arrived after " << delivery_delay << " ms.\n";
            bubble("Transmission successful. Creating a new one!"); // Cosmetic modification.
            delete ttmsg; // We make sure of deleting the already used object.

            if (numSent != 19000){
                EV << "Generating another message: ";
                l1t3Msg *newmsg = generateMessage();
                EV << newmsg << endl;
                if (newmsg->getDestination_column()==col){
                    forwardMessageVertically(newmsg);
                // If we're already in the NEW target column, we forward on the vertical ring.
                } else {
                    forwardMessageHorizontally(newmsg);
                // If we're not in the NEW target column, we forward on the horizontal ring.
                }
                numSent++;
            }
        } else { //If we're not in the destination row...
            // We forward on the vertical ring.
            forwardMessageVertically(ttmsg);

        }
    } else { // If we're not in the destination column...
        // We forward on the horizontal ring.
        forwardMessageHorizontally(ttmsg);
    }
}

l1t3Msg *l1t3::generateMessage()
{
    // Produce source and destination addresses.
    int n = getVectorSize(); // How many modules are there in the network?
    int dest = intuniform(0, n-2);// Integer uniformly distributed number between 0 and n-2.
    // n modules in the network
    // - 1 (accounting for indexes as they go from 0 to n-1)
    // - 1 (we can't send to self)
    if (dest >= ind)
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

    int dest_row = (int) dest/5; // Obtaining destination row
    int dest_col = (int) dest%5; // Obtaining destination row

    char msgname[20];// Message name.
    sprintf(msgname, "Msg-from-%d-to-%d", ind, dest);
    // Save message name on message name variable.

    // Create message object and set source, destination row and destination column field.
    l1t3Msg *msg = new l1t3Msg(msgname);
    msg->setSource(ind);
    msg->setDestination_column(dest_col);
    msg->setDestination_row(dest_row);
    return msg;
}

void l1t3::forwardMessageHorizontally(l1t3Msg *msg)
{
    EV << "Forwarding message " << msg << " horizontally\n";
    if (ind == 19 || ind == 18){
        // Only connections 18-19 and 19-15 are in the horizontal ring.
        // We use the parameter L for the channel delay calculation.
        msg->setDelivery_delay(msg->getDelivery_delay()+channel_delay);
    } else {
        // For the rest of the connections, we use 10 ms.
        msg->setDelivery_delay(msg->getDelivery_delay()+10);
    }
    // All out[0] gates are horizontal.
    send(msg, "out", 0);
}

void l1t3::forwardMessageVertically(l1t3Msg *msg)
{
    EV << "Forwarding message " << msg << " vertically\n";
    if (ind == 19 || ind == 14){
        // Only connections 14-19 and 19-24 are in the vertical ring.
        // We use the parameter L for the channel delay calculation.
        msg->setDelivery_delay(msg->getDelivery_delay()+channel_delay);
    } else {
        // For the rest of the connections, we use 10 ms.
        msg->setDelivery_delay(msg->getDelivery_delay()+10);
    }
    // All out[1] gates are vertical.
    send(msg, "out", 1);
}

void l1t3::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in the console.
    EV << "Sent:     " << numSent << endl;
    double Module21_Avg = delayStats.getMean();
    double Module21_Std = delayStats.getStddev();
    double Module21_Min = delayStats.getMin();
    double Module21_Max = delayStats.getMax();

    recordScalar("#sent", numSent);
    recordScalar("Mod21Avg", Module21_Avg);
    recordScalar("Mod21Std", Module21_Std);
    recordScalar("Mod21Min", Module21_Min);
    recordScalar("Mod21Max", Module21_Max);
    delayStats.recordAs("Delivery delay");
}
