//Package lab4;


//the volatile parameter -> everytime when you want to get the value you need to call par("delayTime")

simple l4_tx // Tx module.
{
    parameters:
        volatile double delayTime @unit(s);   // delay before sending back message        
    gates:
        input in;
        output out;
}

simple l4_rx // Rx module.
{
    parameters:
        @display("i=block/process");
    gates:
        input in;
        output out_to_tx;
        output out_to_link;
}

simple l4_link // Link module.
{
    parameters:
        double L @unit(ms); // Parameter for variable delay, accessible to network.
        volatile int random_gate; // Parameter for random uniform integer value between 0 and 1.
        double ttx @unit(s); // Parameter for fixed transmission time.
        @display("i=block/routing");
    gates:
        input in[];//horizontal and vertical ->two input ports
        input ftx;
        input frx;
        output out[];//horizontal and vertical ->two output ports
        output trx;
}

module l4 // Compound module.
{
    parameters:
        @display("i=misc/node_vs,black;p=,,m,5,50,50;bgb=145,145");

    gates:
        input in[];
        output out[];

    submodules:
        rx: l4_rx {// the submodule 'rx' of the compound module is a simple module l4_rx
            @display("p=90,100");
        }
        link: l4_link {
            @display("p=60,60");
        }
        tx: l4_tx {
            @display("p=30,100;is=s");
        }
    connections:
        rx.out_to_tx --> tx.in;
        tx.out --> link.ftx;
        link.trx --> rx.in;
        in++ --> link.in++; //horizontal direction
        in++ --> link.in++; // vertical direction
        rx.out_to_link --> link.frx;
        link.out++ --> out++;
        link.out++ --> out++;
}


network lab4
{
    parameters:
        double L @unit(ms); // Parameter for variable delay, accessible to network.

        @display("bgb=67,77");
    types:
        channel shortChannel extends ned.DelayChannel
        {
            delay = 10ms;
        }
        channel longChannel extends ned.DelayChannel
        {
            delay = L; //@unit(ms); defined in the .ini file as the argument of the nrtwork modeling
        }

    submodules:
        manh[25]: l4;
    connections:
        for i=0..24 {
            // Horizontal gates are filled first.
            
            //If row is odd and column is not four
            manh[i].out++ --> shortChannel --> manh[i+1].in++ if ((i/5)%2 == 0) && (i%5!=4) && (i!=18);
            //If row is odd and column is four
            manh[i].out++ --> shortChannel --> manh[i-4].in++ if ((i/5)%2 == 0) && (i%5==4) && (i!=19);
            //If row is even and column is zero
            manh[i].out++ --> shortChannel --> manh[i-1].in++ if ((i/5)%2 != 0) && (i%5!=0);
            //If row is even and column is zero
            manh[i].out++ --> shortChannel --> manh[i+4].in++ if ((i/5)%2 != 0) && (i%5==0);

            //LongChannel on modules adjacent to module 19.
            
            //Link 18-19.
            manh[i].out++ --> longChannel --> manh[i+1].in++ if ((i/5)%2 == 0) && (i%5!=4) && (i==18);
            //Link 19-15.
            manh[i].out++ --> longChannel --> manh[i-4].in++ if ((i/5)%2 == 0) && (i%5==4) && (i==19);

	        //OMNeT++ does not return the result of "/" as an int, so we need to substract
        	//the mod of the division, divided by the same dividend in order to check if 
        	//any particular row possesses a certain value.

            // Vertical gates are filled second.
            
            //If column is even and row is not four
            manh[i].out++ --> shortChannel --> manh[i+5].in++ if (((i%5)%2 == 0) && ((i/5)-i%5/5)!=4 && (i!=14) && (i!=19));
            //If column is even and row is four
            manh[i].out++ --> shortChannel --> manh[i-20].in++ if ((i%5)%2 == 0) && (((i/5)-(i%5)/5)==4);
            //If column is odd and row is not zero
            manh[i].out++ --> shortChannel --> manh[i-5].in++ if ((i%5)%2 != 0) && (((i/5)-i%5/5)!=0);
            //If column is odd and row is zero
            manh[i].out++ --> shortChannel --> manh[i+20].in++ if ((i%5)%2 != 0) && (((i/5)-i%5/5)==0);

			//LongChannel on modules adjacent to module 19.
			//Links 14-19 and 19-24.
            manh[i].out++ --> longChannel --> manh[i+5].in++ if (((i%5)%2 == 0) && ((i/5)-i%5/5)!=4 && ((i==14) || (i==19)));

        }

}
