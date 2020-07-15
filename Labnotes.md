
# Manhattan-Network-Modeling
Manhattan Network Modeling built by OMNET++


## Preliminary

- G is the group number.

- LongChannel or disable channels just around node G.



## What Is Our Goal For All Labs

- Under a specific Routing way

    - set arguments for LongChannel delay Parameter *among L={10,30,50,70,90}) 10 simulations for each*

- Find and record:

    - **Average Delivery Delay** of messages to a specific node (e.g. G+2) , as a function of L.

    对于G+2这个点来说，记录所有到达这个点的信息所经历的delay，算平均值 ----> 一个信息到达一个点的总延迟。

    - **Average Hop-Count of messages** of every node. (This is an information for later use)

    对于每一个点来说，记录所有到达这个点的信息所经过的路由次数，算平均值 ----> 一个信息到达一个点的路由次数平均值。

- Study the network performance using these three indeces:

    - Pick an appropriate argument for *Tia* such that 95% confidence interval of the estimate of **average Q**, out of 10 runs, loosely falls between 10%-50% of qlimit (Fix L)

    - Plot the **Average msg Delay** of node 'G+2' and its 99% confidence interval as a function of *Ttx*. (You pick 5 appropriate Ttx) using gnuplot.

    - Pick the **Q-loss ratio** with its 99% confidence interval as a function of Ttx. (consider both msg+ack, only due to full queue not HopCount)

*Q here means the queue at the output transmission from node 0-> node 1
*Ttx means the time duration to pop the msg from the queue at the link layer to send it out


## Lab1

- Stop the simulation when any node sent G*1000 MESSAGES.

- Random Routing Method:

    1.Run 5*10 simulations with 5 Different arguments for 'longChannel delay' (L={10,30,50,70,90}) to find:

        - Average Delivery Delay of the messages that arrive at the node G+2, as a function of parameter L.


    2.Find Average Hop Count of the messages that arrive at every node, with different channel conditions around G:
    
    记录 -> 对于每一个点来说，所有到达这个点的信息，它经过多少个路由器->算平均值。

        a. All operational with short channel.
        
        b. Disable the links between G's column predecessor & column successor\
        
        c. Disable all vertical links of G's column.


- Fix Routing Method: (going in the same row/coloumn until intercepts 探测到 the target colomn/row):

    Repeat 1.
    
    

## Lab2 

- Stop the simulation when each node sent G*1000 MESSAGES.

- 0.1 Loss probability at the linkLayer module.

- Stop and Wait protocol at the tx/rx layer modules ->retx a lost message.

- Delivery Delay: tx + retx (Do the statistics at node G+2)



## Lab3

- Stop the simulation when each node sent G*1000 MESSAGES.

- FSM: finite state machine to implement the 'Stop & Wait' protocol at tx/rx layer (modules).

- Every node's tx starts to send the first message->every time take a random time Tia=0 before send it.

- msg has sequence number 0/1/0/1.

- tx:

    - starts to send first msg with seq=0

    - set timeout
    
    - timeout retx
    
    - discard out-of-order ACKS
   
- rx:

    - starts to wait first msg with seq=0
    
    - if receive msg -> swap addresses and mark it as ACK 0 -> wait for msg 1
    
    - if receive duplicate msg -> ignore -> not change state -> return it to sender?
    
    - if receive an ACK with seqNo0/1 -> send an 'ACK(0/1)received' msg to tx module on the same node.
    
    
- Link:

    - receive and deliver
    
    - seq=0 ROW/COL Routing
    
    - seq=1 RANDOM Routing

    - discard msg with hopcount>30, no loss prob
    
    - when msg arrives its dest -> hopcount->0
    
- Setting Tia=0 -> Record Delivery Delay of node G+2


## Lab4
 
- Stop the simulation when each node sent 5000+G*100 MESSAGES.

- Buffering at the link layer with limit q=ceil[10+G/3] (discard if over)

- two buffer queues at link layer for Row+Column outgoing ports

- Random time to send the new msg-> Tia = exp(G/100)

- longChannel delay L=0.03

- self-msg for each queue to pop the msg every Ttx=0.05 seconds to send it out

- Pick an appropriate argument for *Tia* such that 95% confidence interval of the estimate of **average Q**, out of 10 runs, loosely falls between 10%-50% of qlimit (Fix L)

- Plot the **Average msg Delay** of node 'G+2' and its 99% confidence interval as a function of *Ttx*. (You pick 5 appropriate Ttx) using gnuplot.

- Pick the **Q-loss ratio** with its 99% confidence interval as a function of Ttx. (consider both msg+ack, only due to full queue not HopCount)

*Q here means the queue at the output transmission from node 0-> node 1 *

*Ttx means the time duration to pop the msg from the queue at the link layer to send it out*


