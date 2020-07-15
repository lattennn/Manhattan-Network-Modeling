simple l1t2
{
    parameters:
        @display("i=block/routing");
        double L @unit(ms); // Parameter for variable delay, accessible to modules.
    gates:
        input in[];
        output out[];
}

network lab1_task2
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
        manh[25]: l1t2;
    connections:
        for i=0..24 
        {
        	//(i/5) -> Row
        	//(i % 5) -> Column
        	
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
        	
        	
        	// Vertical gates are filled first.
        	//OMNeT++ does not return the result of "/" as an int, so we need to substract
        	//the mod of the division, divided by the same dividend in order to check if 
        	//any particular row possesses a certain value.        	

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

