#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l2t1Msg_m.h" // Data message type forwarded between modules.
#include "l2t1RxToTxMsg_m.h" // Message forwarded from rx to tx as a Wake up.

using namespace omnetpp;

class l2t1_rx : public cSimpleModule
{
    private:
        long numReceived; // Number of sent packages from the module object.
        cHistogram delayStats; // OMNeT++ objects for data processing and statistics.
        cOutVector delayVector; // OMNeT++ objects for data processing.
        int ind; // Module number.

    protected:
        virtual void initialize() override;
        // Initialization function.
        virtual void handleMessage(cMessage *msg) override;
        // Message handling function.
        virtual void wakeUp();
        // Send message to Tx
        virtual void finish() override;
        // Calculate statistics, conclude simulation.
};

Define_Module(l2t1_rx);

void l2t1_rx::initialize()
{
    // Variable initialization.
    numReceived = 0; // Setting it to 0.
    WATCH(numReceived); // Useful tool for debugging.
    ind = getParentModule() -> getIndex(); // Similar as used in Link.
    // Statistical analysis classes provided by OMNeT++.
    delayStats.setName("delDelStats"); // Setting a name to the histogram
    delayVector.setName("delivery_delay"); // Setting a name to the vector collecting
    if (ind == 0) wakeUp(); // If we are module 0, we start the simulation.
 }

void l2t1_rx::handleMessage(cMessage *msg)
{
    l2t1Msg *ttmsg = check_and_cast<l2t1Msg *>(msg);
    // We make sure the message we're receiving is of type l2t1Msg, and we convert it to this type if so.
    int delivery_delay = ttmsg->getDelivery_delay(); //Obtain delivery delay from message data field.
    if (ind == 21){ // We're interested in saving packet delivery delay on module 21.
    // We obtain the desired value and proceed to record it in our data structures.
        delayVector.record(delivery_delay);
        delayStats.collect(delivery_delay);
    }
    delete ttmsg; // We make sure of disposing of the already used object.
    numReceived++; // We increase the number of received packages.

    if (numReceived != 19001){ //Condition to stop the simulation.
    // If we've received 19001 messages, we've transmitted 19000 messages.
    wakeUp(); // Calling tx to wakeUp, and send a new message..
    }
}

void l2t1_rx::wakeUp()
{
    l2t1RxToTxMsg *wakeUpMsg = new l2t1RxToTxMsg();
    // We create a new message that we will proceed to transmit towards tx.
    send(wakeUpMsg,"out"); // Send message to tx.
}

void l2t1_rx::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in console.
    EV << "Sent:     " << numReceived << endl;
    double Module21_Avg = delayStats.getMean();
    double Module21_Std = delayStats.getStddev();
    double Module21_Min = delayStats.getMin();
    double Module21_Max = delayStats.getMax();

    recordScalar("#Received", numReceived);
    recordScalar("Mod21Avg", Module21_Avg);
    recordScalar("Mod21Std", Module21_Std);
    recordScalar("Mod21Min", Module21_Min);
    recordScalar("Mod21Max", Module21_Max);
    delayStats.recordAs("Delivery delay");
}
