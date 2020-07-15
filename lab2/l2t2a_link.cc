#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l2t2a_m.h"

using namespace omnetpp;
using namespace std;

class l2t2a_link : public cSimpleModule
{
  private:
    double channel_delay; // Delay variable for average delay calculation.
    int ind, row, col; // Compound module index, row and column.

  protected:
    virtual void forwardMessageHorizontally(l2t2aMsg *msg);
    //Function for message forwarding on horizontal ring output gate.
    virtual void forwardMessageVertically(l2t2aMsg *msg);
    //Function for message forwarding on vertical ring output gate.
    virtual void forwardToRx(l2t2aMsg *msg);
    //Function to forward to internal Rx submodule.
    virtual void initialize() override;
    //Function for initialization of modules.
    virtual void handleMessage(cMessage *msg) override;
    //Function in which we decide what to do given a message.
};

Define_Module(l2t2a_link); //Module registration

void l2t2a_link::initialize()
{
    // Variable initialization.
    channel_delay = par("L");  // Channel delay parameter taken from ini file.
    ind = getParentModule() -> getIndex();
    row = (int) ind/5; // i, starting from 0.
    col = ind%5; // j, starting from 0.
}

void l2t2a_link::handleMessage(cMessage *msg)
{
   if (uniform(0,1)<0.1){ // Probability of loss is 10%.
       delete msg; // We delete if value obtained is less than 0.1
   } else {
       l2t2aMsg *ttmsg = check_and_cast<l2t2aMsg *>(msg);
       // Check that the received message belongs to class l2t2aMsg, our predefined message class.
        if (ttmsg->getDestination_column() == col) { // If we are in the destination column,
            if (ttmsg->getDestination_row() == row){ // And in the destination row
                // It means we've reached the destination.
                forwardToRx(ttmsg); // We forward to rx to get statistics from data message.
            } else { //If we're not in the destination row...
                // We forward on the vertical ring.
                forwardMessageVertically(ttmsg);
            }
        } else { // If we're not in the destination column...
            // We forward on the horizontal ring.
            forwardMessageHorizontally(ttmsg);
        }
   }
}

void l2t2a_link::forwardMessageHorizontally(l2t2aMsg *msg)
{
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

void l2t2a_link::forwardMessageVertically(l2t2aMsg *msg)
{
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

void l2t2a_link::forwardToRx(l2t2aMsg *msg)
{
    send(msg, "trx"); // Send to gate connected to rx.
}
