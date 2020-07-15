#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "l2t1Msg_m.h" // Data message type forwarded between modules.
#include "l2t1RxToTxMsg_m.h" // Message forwarded from rx to tx as a Wake up.

using namespace omnetpp;

class l2t1_tx : public cSimpleModule
{
    private:
        int ind; // Module number.

    protected:
        virtual void initialize() override;
        // Initialization function.
        virtual l2t1Msg *generateMessage();
        // Generate message to tx.
        virtual void handleMessage(cMessage *msg) override;
        // Message handling function.
        virtual void forwardToLink(l2t1Msg *msg);
        // Sending generated messages out to link layer.
};

Define_Module(l2t1_tx); //Module definition.

void l2t1_tx::initialize()
{
    // Variable initialization.
    ind = getParentModule() -> getIndex(); // Get module index.
}

void l2t1_tx::handleMessage(cMessage *msg)
{
    l2t1RxToTxMsg *received_msg = check_and_cast<l2t1RxToTxMsg *>(msg);
    // We make sure the message we're receiving is of type l2t1Msg, and we convert it to this type if so.
    delete received_msg;
    // Dispose of the message as it does not possess any important information.
    l2t1Msg *gen_msg = generateMessage(); // We generate a new message to be transmitted.
    forwardToLink(gen_msg); // We forward the newly generated message over to link layer.
}

void l2t1_tx::forwardToLink(l2t1Msg *msg)
{
    send(msg,"out"); // Send to link.
}

l2t1Msg *l2t1_tx::generateMessage()
{
    // Produce source and destination addresses.
    int n = getParentModule() -> getVectorSize(); // How many modules are there in the network? Store in n.
    int dest = intuniform(0, n-2);// Integer uniformly distributed number between 0 and n-2.
    // n modules in the network
    // - 1 (accounting for indexes as they go from 0 to n-1)
    // - 1 (we can't send to self)
    if (dest >= ind) dest++;
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

    int dest_row = (int) dest/5; // Obtaining destination row.
    int dest_col = (int) dest%5; // Obtaining destination column.

    char msgname[20];// Message name.
    sprintf(msgname, "Msg-from-%d-to-%d", ind, dest);
    // Save message name on message name variable.

    // Create message object and set source, destination row and destination column field.
    l2t1Msg *msg = new l2t1Msg(msgname);
    msg->setSource(ind);
    msg->setDestination_column(dest_col);
    msg->setDestination_row(dest_row);
    return msg;
}
