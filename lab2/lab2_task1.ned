module l2t1
{
    parameters:
        @display("i=misc/node_vs,black;p=,,m,5,50,50;bgb=235,168"); // Cosmetic change. 
        // Makes the nodes form a matrix.
        double T_ia @unit(ms); // Interarrival time parameter between rx and tx.

    gates:
        input in[]; // Gates of the Compound module where messages from other Compound modules are received
        output out[]; // Gates of the Compound module that send messages to other Compound modules.

    submodules:
        rx: l2t1_rx { // Submodule class l2t1_rx is referred as "rx",
            @display("p=45,113");
        }
        link: l2t1_link { // Submodule class l2t1_link is referred as "link",
            @display("p=159,113");
        }
        tx: l2t1_tx { // Submodule class l2t1_tx is referred as "tx".
            @display("p=98,33");
        }
    connections:
        rx.out --> {  delay = T_ia; } --> tx.in; // Connect rx output to tx input with a delay T_ia
        tx.out --> link.ftx; // Connect tx output to link internal input.
        link.trx --> rx.in; // Connect link internal output to rx input.        
        in++ --> link.in++; // Connect a new instance of the compound module input to a new instance of the link input.
        in++ --> link.in++; // Same as before.
        link.out++ --> out++; // Connect a new instance of the link output to a new instance of the compound module output.
        link.out++ --> out++; // Same as before.       
}

network lab2_task1
{
    parameters:
        double L @unit(ms); // Parameter for variable delay, accessible to network.

    types:
        channel shortChannel extends ned.DelayChannel
        {
            delay = 10ms;
        }
        channel longChannel extends ned.DelayChannel
        {
            delay = L; //@unit(ms);
        }

    submodules:
        manh[25]: l2t1;
    connections:
        for i=0..24 {
            // Horizontal gates are filled first.
            //If row is odd and column is not four
            manh[i].out++ --> shortChannel --> manh[i+1].in++ if ((i/5)%2 != 0) && (i%5!=4) && (i!=18);
            //If row is odd and column is four
            manh[i].out++ --> shortChannel --> manh[i-4].in++ if ((i/5)%2 != 0) && (i%5==4) && (i!=19);
            //If row is even and column is zero
            manh[i].out++ --> shortChannel --> manh[i-1].in++ if ((i/5)%2 == 0) && (i%5!=0);
            //If row is even and column is zero
            manh[i].out++ --> shortChannel --> manh[i+4].in++ if ((i/5)%2 == 0) && (i%5==0);

            //LongChannel on modules adjacent to module 19.
            //Link 18-19.
            manh[i].out++ --> longChannel --> manh[i+1].in++ if ((i/5)%2 != 0) && (i%5!=4) && (i==18);
            //Link 19-15.
            manh[i].out++ --> longChannel --> manh[i-4].in++ if ((i/5)%2 != 0) && (i%5==4) && (i==19);

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

