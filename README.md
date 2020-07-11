

# Manhattan-Network-Modeling
Manhattan Network Modeling built by OMNET++


# Basics about Omnet++

An Omnet++ model (an network) consists (composed) of modules that communicate with **message passing** *via connection*:

  + simple modules: written in C++, using the simulation class library
  
  + compound modules: Group of simple modules 
  

In order to simulate a network in Omnet we need three kinds of files:

1. Network Description File (.ned)
2. Network Configuration File (.ini)
3. Source file (.cc)


## Network Description File (.ned)

```c++
Package package_name; // you define the name of your package (It's also the name of the omnet++ project)

simple Anode  // command `simple` to definea simple module  -> The first letter should be upper case
{
    gates:          // `gates` to specify the ports of the node
        input in;   // variable -> input port named 'in'
        output out; // variable -> output port named 'out'
}

network Anetwork    // `network`define the network named Anetwork -> The first letter should be upper case
{ 
    submodules:
        host1: Anode;  //Assign a simple module Anode to host1
        host2: Anode;
        
    connections:
        host1.out --> host2.in;
        host2.out --> host1.in;
}
```

## Network Configuration File (.ini)

The network configuration file (also called initialization file) contains the **network**. And here the example, the name of my network is called Anetwork.

omnetpp.ini:
```
[General]
network = Anetwork;
```

## Network Source File (.cc)

```c++
#include <string.h>
#include <omnetpp.h>

using namespace omnetpp; 
//using the namespace we can invoke/import all the commands and functions and methods that are presented in the `omnetpp.h` header

//this name `Anode` must be the exact name of the node you define in the ned. file
class Anode: public cSimpleModule  // here we derive a class from cSimpleModule base class
{
protected:                           //we have two protected methods
    void initializd() override; 
    void handleMessage(cMessage *msg) override;
}; 

Define_Module(Anode); // you need to register the class `Anode` with OMNET++ using the `Define_Module()`method

void Anode::initialize()
{
    if(strcmp("host2",getName())==0){
        cMessage *msg = new cMessage("Hello there!");
        send(msg,"out"); //host2 send the msg using the gate named "out" 
    }
}

void Anode::handleMessage(cMessage *msg)
{
    send(msg,"out"); //send back the message from "output" gate 
}
``` 

## Start OMNET++

+ double click mingwenv/cmdline -> type **omnetpp** 

+ File -> new ->OMNET++ Project

+ Give a name to the project -> next

+ Select Empty project with 'src' and 'simulation' folders -> finish

+ Go to the working directory in the 'Project Explorer'

+ In the 'src' directory we have the description file -> package.ned -> open it

+ We have the 'Design' & 'Source' part

+ right click the source -> new -> Network Description File (NED) -> the first file we want to create -> give it a name -> ...ned

+ hit -> NED file with one item -> it will appear a package name and a network name

+ If we go to the design part we can see the GUI of the source code -> package name and your network

+ We have two possibilities to create nodes -> 

    - First method you just click on the simple module icon (in the Types) THEN click inside the canvas -> If we go to 'source' we will see inside the 'network' it automatically added that simple module and it's called unnamed->we can change it from either parts.
    
    - Second you just go to the cource code write everything you want.    

+ We need to create two hosts -> in design part you just need to drag it and move it into the network box -> then change the name.
  
```c++ 
//What the 'design' part did

package hello_world;

network My_hello_world
{
    types:
        simple Anode
        {
            gates:
                input input_gate;
                output output_gate;
        }
    submodules:
        host1: Anode;
        host2: Anode;
    connection:
        host1.output_gate --> host2.input_gate;
        host2.output_gate --> host1.input_gate;
        
//But notice when you move the module in the 'design' part, in the 'source' part -> @display changes to reflect the motion
}

```

+ Now we need two computers(hosts) to communicate -> connect to each other -> you need ports/gates -> define in the simple module

+ From the design part you just need to click on 'connection' and on 1 module drag to another and release (before the connection you need to have ports) -> click on the module -> set the connection 

+ Now lets go the 'src' directory and right click -> new -> New Source File -> give it a name.cc

+ The C++ file is the file that contains the logic of the network.

+  First import 
   
   ```c++
   #include <omnetpp.h>
   using namespace omnetpp; // Call and use all the omnetpp libraries in the omnetpp.h
   ```
   
+ You can find the folder of omnetpp.h -> go to your omnetpp-5.6 folder->include->omnetpp.h and omnetpp libraries.

+ Next we define our class -> as a derived(inherited) class of the cSimpleModule -> the base class -> it comes from the omnetpp library (namespace) that we have imported through the omnetpp header (include omnetpp.h)

+ **Again, the name of the class should be the name we defined in the .ned file.

+ We make two methods inside the class in the .cc file. (Example above)

+ We define the methods -> 返回类型 class名::class里那个method的名()

+ Last we need the initialization/configuration file -> .ini file

  *(relase mode to accelerate the simulation)*
  
+ Remeber before defining the method and after the class -> register the class 'Anode' with OMNET++ -> Define_Module(Anode);

+ Remeber here you will get error ---> change every 'host' into 'Host' -> it's case sensitive -> the name of the module should be Upper!

+ Done


