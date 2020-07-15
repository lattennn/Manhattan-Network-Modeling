#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l2t2a_m.h" // Our message type.

using namespace omnetpp;

long rx_SEQ[25]; // Array to save last received sequence number, the index corresponds to the source.
char ackname[20]; // Array used to save the Ack message name.


class l2t2a_rx : public cSimpleModule
{
    private:
        int ind, ACK_dest, rx_seq; // Index of compound module, destination of ACK and sequence number of received message.
        unsigned long delay_tot; // Total delay of all messages.
        long received = 0; // Total received messages.

    protected:
        // Function to be called when a message is received.
        virtual void handleMessage(cMessage *msg) override;
        // Function for initializing variables.
        virtual void initialize() override;
        // Function to be performed when simulation is finished.
        virtual void finish() override;

};

Define_Module(l2t2a_rx);

void l2t2a_rx::initialize()
{
    ind = getParentModule() -> getIndex(); // Index of compound module.
}

void l2t2a_rx::handleMessage(cMessage *msg)
{
   l2t2aMsg *received_msg = check_and_cast<l2t2aMsg *>(msg); // The only types of message tx receives are either data or ACK messages.
   // There are no self messages.
   if (received_msg->getKind() == 1){ //Message kind 1 is ACK.
       delete received_msg; // We need no information from the ACK, so we delete it.
       l2t2aMsg *ack_rxed = new l2t2aMsg("ack_rxed",2); // Then, we sent an ACK_received message to tx for it to cancel the timeout.
       // It has kind 2.
       send(ack_rxed,"out_to_tx"); // Send out to tx.
   } else {
       ACK_dest = received_msg -> getSource(); // Destination of ACK is sender of data message.
       sprintf(ackname,"Ack from %d to %d - SEQ %d",ind,ACK_dest, received_msg->getSeq()); //Give name to ackname string.
       l2t2aMsg *ack = new l2t2aMsg(ackname,1); // Create an ACK message with name ackname and kind 1.
       ack -> setDestination_column(ACK_dest%5); // Set destination column,
       ack -> setDestination_row((int)ACK_dest/5); // and destination row.
       ack -> setSource(ind); // Source of ACK is self.
       rx_seq = received_msg -> getSeq(); // Get the sequence number of the received message.
       if (ind == 21){ // If we're module G+2,
           if (rx_SEQ[ACK_dest] != rx_seq){
               // And if in index of source of data message the last received sequence number
               // corresponds to the actual one, it means we are receiving a retransmission,
               // and our previously sent ACK didn't arrive.
               // We have received this data already, so we discard if the received number
               // in the source index is equal to the last saved one.
               rx_SEQ[ACK_dest] = rx_seq;
               // We save the just received message in the index number corresponding to the
               // source in our rx_SEQ array.
               int delay = received_msg -> getDelivery_delay(); //  Obtain delivery delay.
               delay_tot += delay; // Save it to total amount.
               received++; // Add to received counter.
           }
       }
       delete received_msg; // Delete received message, done for all cases.
       send(ack, "out_to_link"); // Send on connection to link.
   }
}


void l2t2a_rx::finish()
{
    // Called when OMNeT++ perceives no more events happening.
    // Statistics are collected and some are shown in the console.
    if (ind == 21){
    double Module21_Avg = (double) delay_tot/received;
    recordScalar("Mod21Avg", Module21_Avg);
    EV << "Total delay is " << delay_tot << " and I received " << received << endl;
    }
}


