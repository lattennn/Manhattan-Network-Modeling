simple l3_tx // Tx module.
{
    parameters:
        volatile double delayTime @unit(s);   // delay before sending back message        
    gates:
        input in;
        output out;
}

simple l3_rx // Rx module.
{
    parameters:
        @display("i=block/process");
    gates:
        input in;
        output out_to_tx;
        output out_to_link;
}

simple l3_link // Link module.
{
    parameters:
    	double L @unit(ms); // Parameter for variable delay, accessible to network.
	    @display("i=block/routing");
	gates:
	    input in[];
        input ftx;
        input frx;        
        output out[];
        output trx;
}
    	
        

module l3 // Compound module.
{ 
    parameters:
        @display("i=misc/node_vs,black;p=,,m,5,50,50");    

    gates: 
        input in[];
        output out[];
            
    submodules:
        rx: l3_rx;
        link: l3_link;        
        tx: l3_tx;
        
     connections:
        rx.out_to_tx  --> tx.in;
        tx.out --> link.ftx;
        link.trx --> rx.in;        
        in++ --> link.in++;
        in++ --> link.in++;
        rx.out_to_link --> link.frx;                
        link.out++ --> out++;
        link.out++ --> out++;     
}


network lab3
{
    parameters:
    	double L @unit(ms); // Parameter for variable delay, accessible to network.
    	
    types:
        channel shortChannel extends ned.DelayChannel { // Channel for links not connected to G+2.
            delay = 10ms;
        }        
        channel longChannel extends ned.DelayChannel { // Channel for links connected to G+2.
            delay = L; //@unit(ms);
		}

    submodules:
        manh[25]: l3;
    connections:
        for i=0..24 
        {
        	//j = (i/5) //-> Row
        	//k = (i % 5) //-> Column
        	
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