#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l4Msg_m.h"

using namespace omnetpp;
using namespace std;

#define G 19 // Group number.

class l4_link : public cSimpleModule
{
  private:
    double q_loss; // Variable to store losses at queue insertion.
    int ind, row, col, qlimit; // Compound module index, row, column and queue limit.
    cQueue queue_0; // Queues
    cQueue queue_1;
    cMessage *pop0; // Self messages for each queue
    cMessage *pop1;
    simtime_t last_change, ttx; // Auxiliary variable to record changes in queue length random process, and transmission time.
    double Q_tot, Q; // Total average of the queue length random process, and cumulative value of it

  public:
      l4_link();
      virtual ~l4_link();

  protected:
    virtual void forwardMessageHorizontally(l4Msg *msg);
    //Function for message forwarding on horizontal ring output gate.
    virtual void forwardMessageVertically(l4Msg *msg);
    //Function for message forwarding on vertical ring output gate.
    virtual void forwardToRx(l4Msg *msg);
    //Function to forward to internal Rx submodule.
    virtual void initialize() override;
    //Function for initialization of modules.
    virtual void handleMessage(cMessage *msg) override;
    //Function in which we decide what to do given a message.
    void finish() override;
    //Function runned after the simulation finishes.
};

Define_Module(l4_link); //Module registration

l4_link::l4_link(){
    pop0 = pop1 = nullptr;
}

l4_link::~l4_link(){
    delete pop0;
    delete pop1;
}

void l4_link::initialize()
{
    // Variable initialization.
    cQueue queue_0("Hor"); // Queue naming
    cQueue queue_1("Ver");
    last_change = simTime(); // We take the initial time to start counting our random process.
    Q_tot = 0;
    Q = 0;
    q_loss = 0;
    WATCH(Q_tot); // Debugging purposes.
    pop0 = new cMessage("pop0"); // We create the message objects.
    pop1 = new cMessage("pop1");
    ttx = par("ttx"); // Obtain ttx from ini file.
    ind = getParentModule() -> getIndex(); // Index
    row = (int) ind/5; // i, starting from 0.
    col = ind%5; // j, starting from 0.
    qlimit = ceil(10+G/3); // Limit of the queue.
}

void l4_link::handleMessage(cMessage *msg)
{
    if (msg == pop0) { // If we received the self message for queue 0.
        if (ind == 0){ // If we're index 0,
            Q_tot += (SIMTIME_DBL(simTime())-SIMTIME_DBL(last_change))*queue_0.getLength();
            // We record the value of the process for the time it took for last change to occur.
        }
        l4Msg *qMsg = (l4Msg *)queue_0.pop(); // We pop the first message in the queue.
        if (ind == 0) last_change = simTime(); // and update the time since last change.
        if (!queue_0.isEmpty()){ //If there are more messages in the queue
            scheduleAt(simTime()+ttx, pop0); // Reschedule pop0.
        }
        forwardMessageHorizontally(qMsg); // We send the obtained message horizontally.
    } else if (msg == pop1) { // Same as with queue 0.
        // No update or random process value is calculated here as the connection from 0 to 1 is only horizontal.
        l4Msg *qMsg = (l4Msg *)queue_1.pop();
        if (!queue_1.isEmpty()){
            scheduleAt(simTime()+ttx, pop1);
        }
        forwardMessageVertically(qMsg); // We forward vertically in queue 1.
    } else {
        l4Msg *ttmsg = check_and_cast<l4Msg *>(msg);
        // Check that the received message belongs to class l4Msg, our predefined message class.
       if (ttmsg->getHopcount() == 30){
           delete ttmsg; // We delete if 30 hops have passed.
       } else {
           ttmsg->setHopcount(ttmsg->getHopcount()+1); // If 30 hops have not occurred, we increase by 1 the number of hops.
           if (ttmsg->getSEQ() == 0){ // For the ACKs and Data messages of seq 0.
               if (ttmsg->getDestination_column() == col) { // If we are in the destination column,
                   if (ttmsg->getDestination_row() == row){ // And in the destination row
                    // It means we've reached the destination.
                       forwardToRx(ttmsg); // We forward to rx to get statistics from data message.
                    } else { //If we're not in the destination row...
                    // We add message to the vertical output queue.
                        if (queue_1.getLength() <= qlimit){ // If we have less than q limit messages.
                            queue_1.insert(msg); // We insert the new message.
                            if (!pop1 -> isScheduled()) scheduleAt(simTime()+ttx,pop1); // If the queue is empty, no pop message was set, so we set it.
                        } else{
                            delete msg; // If we're over qlimit, we delete message.
                            q_loss++; // And increase the loss variable.
                        }
                    }
                } else { // If we're not in the destination column...
                    // We add message to the horizontal output queue.
                    if (queue_0.getLength() <= qlimit){ // Similar to the vertial output queue
                        if (ind == 0){ // We update the state of the process.
                            Q_tot += (SIMTIME_DBL(simTime())-SIMTIME_DBL(last_change))*queue_0.getLength();
                        }
                        queue_0.insert(msg);
                        if (ind == 0) last_change = simTime(); // We update last hange.
                        if (!pop0 -> isScheduled()) scheduleAt(simTime()+ttx,pop0);
                    } else{
                        delete msg;
                        q_loss++;
                    }
                }
           } else { // For the ACKs and Data messages of seq 1.
               if (ind == ttmsg-> getDestination()) forwardToRx(ttmsg);
               else {
                   int rg = par("random_gate"); // We forward at random,
                   if (rg == 0){ // If we have to go horizontally, we go with queue 0,
                       if (queue_0.getLength() < qlimit){
                           if (ind == 0){
                               Q_tot += (SIMTIME_DBL(simTime())-SIMTIME_DBL(last_change))*queue_0.getLength();
                               //EV << "QUEUE STAYED IN " << queue_0.getLength() << " FOR " << SIMTIME_DBL(simTime())- SIMTIME_DBL(last_change) << "seconds.\n";
                           }
                           queue_0.insert(msg);
                           if (ind == 0) last_change = simTime();
                           if (!pop0 -> isScheduled()) scheduleAt(simTime()+ttx,pop0);
                       } else{
                           delete msg;
                           q_loss++;
                       }
                   } else { // If we have to go vertically, we go with queue 1.
                       if (queue_1.getLength() < qlimit){
                           queue_1.insert(msg);
                           if (!pop1 -> isScheduled()) scheduleAt(simTime()+ttx,pop1);
                       } else {
                           delete msg;
                           q_loss++;
                       }
                   }
               }
           }
       }
    }
}

void l4_link::forwardMessageHorizontally(l4Msg *msg)
{
    send(msg, "out", 0); // Send to horizontal gate 0.
}

void l4_link::forwardMessageVertically(l4Msg *msg)
{
    send(msg, "out", 1); // Send to vertical gate 1
}

void l4_link::forwardToRx(l4Msg *msg)
{
    send(msg, "trx"); // Send to gate connected to rx.
}

void l4_link::finish()
{
    recordScalar("Queue loss",q_loss); // Save data for every node.
    if (ind == 0){ // If index is 0, evaluate the average queue length from 0 to 1.
        EV << "Total values is: " << Q_tot << endl;
        Q = Q_tot/(SIMTIME_DBL(simTime()));
        recordScalar("Average Queue length",Q);
    }
}
